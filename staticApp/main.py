import os
import sys
import re
import shutil
import platform
import subprocess
import logging
import hashlib

# IMPORT / GUI AND MODULES AND WIDGETS
# ///////////////////////////////////////////////////////////////
import time

from PySide6 import QtCore
from PySide6.QtWidgets import QMessageBox

from modules import *
from widgets import *
# from modules import raw_model_struct
# from modules import get_node_graph
from pygments import highlight
from pygments.lexers import CppLexer, CLexer
from pygments.lexers import get_lexer_by_name
from pygments.formatters import HtmlFormatter
from threading import Thread

os.environ["QT_FONT_DPI"] = "128"  # FIX Problem for High DPI and Scale above 100%

# SET AS GLOBAL WIDGETS
# ///////////////////////////////////////////////////////////////
widgets = None

watch_name = {"didCodeFileChanged": False, "codefileName": "", "code_md5": ""}


class WatchThread(QThread):
    changed_signal = QtCore.Signal(int)

    def __init__(self):
        """
        :param func: 可调用的对象
        :param args: 可调用对象的参数
        """
        super(WatchThread, self).__init__()

    def run(self):
        global watch_name
        while True:
            if not watch_name["didCodeFileChanged"] and \
                    hashlib.md5(open(watch_name["codefileName"]).read().encode("utf-8")).hexdigest() != watch_name[
                    "code_md5"]:
                print("code is changed. not file name")
                time.sleep(1)
                self.changed_signal.emit(1)
                # self.readFileToCodeBox(None, code_changed=True)
            time.sleep(2)


class MainWindow(QMainWindow):
    def __init__(self):
        """
        init mainWindow
        """
        QMainWindow.__init__(self)

        # SET AS GLOBAL WIDGETS
        # ///////////////////////////////////////////////////////////////
        self.ui = Ui_MainWindow()
        self.ui.setupUi(self)
        global widgets
        widgets = self.ui

        # USE CUSTOM TITLE BAR | USE AS "False" FOR MAC OR LINUX
        # ///////////////////////////////////////////////////////////////
        Settings.ENABLE_CUSTOM_TITLE_BAR = True

        # APP NAME
        # ///////////////////////////////////////////////////////////////
        title = "Static analysis"
        description = "Static Analysis Tools"

        # APPLY TEXTS
        self.setWindowTitle(title)
        widgets.titleRightInfo.setText(description)
        self.picfileName = "./default/welcome_default.png"
        self.errorfilename = "./default/default.txt"
        self.codefileName = "./default/default.c"
        self.astdot = []
        self.cfgdot = []
        self.cdgdot = []
        self.pdgdot = []
        self.dotindex = 0


        self.pic_or_model = "pic"
        self.didCodeFileChanged = False
        self.isDefault = True
        self.code_md5 = hashlib.md5(open(self.codefileName).read().encode("utf-8")).hexdigest()

        # TOGGLE MENU
        # ///////////////////////////////////////////////////////////////
        widgets.toggleButton.clicked.connect(lambda: UIFunctions.toggleMenu(self, True))

        # SET UI DEFINITIONS
        # ///////////////////////////////////////////////////////////////
        UIFunctions.uiDefinitions(self)

        # QTableWidget PARAMETERS
        # ///////////////////////////////////////////////////////////////
        widgets.tableWidget.horizontalHeader().setSectionResizeMode(QHeaderView.Stretch)

        # BUTTONS CLICK
        # ///////////////////////////////////////////////////////////////

        # LEFT MENUS
        widgets.btn_home.clicked.connect(self.buttonClick)
        widgets.btn_model.clicked.connect(self.buttonClick)
        widgets.btn_picShow.clicked.connect(self.buttonClick)

        # EXTRA LEFT BOX
        def openCloseLeftBox():
            """
            open close left box when hide button hit
            """
            UIFunctions.toggleLeftBox(self, True)

        widgets.toggleLeftBox.clicked.connect(openCloseLeftBox)
        widgets.extraCloseColumnBtn.clicked.connect(openCloseLeftBox)

        # EXTRA RIGHT BOX
        def openCloseRightBox():
            """
            open close right box when hide button hit

            warning: Right Hide button is discarded
            """
            UIFunctions.toggleRightBox(self, False)

        # widgets.settingsTopBtn.clicked.connect(openCloseRightBox)

        QImageReader.setAllocationLimit(256)
        self.model_name_list = ["lenet", "squeezenet", "googlenet", "convnet"]
        # SHOW APP
        # ///////////////////////////////////////////////////////////////
        self.show()

        # SET CUSTOM THEME
        # ///////////////////////////////////////////////////////////////
        useCustomTheme = True
        themeFile = "themes/nju_birds_green.qss"

        # SET THEME AND HACKS
        if useCustomTheme:
            # LOAD AND APPLY STYLE
            UIFunctions.theme(self, themeFile, True)

            # SET HACKS
            AppFunctions.setThemeHack(self)

        # SET HOME PAGE AND SELECT MENU
        # ///////////////////////////////////////////////////////////////
        widgets.stackedWidget.setCurrentWidget(widgets.home)

        # SET CODEPAGE
        # ///////////////////////////////////////////////////////////////

        self.endswith = tuple(["c", "cpp", "c++", "h", "hpp"])
        dir_path = '/home/cyrus/'
        self.model = QFileSystemModel()
        self.model.setRootPath(dir_path)
        widgets.codePageFileTreeView.setModel(self.model)
        widgets.codePageFileTreeView.setRootIndex(self.model.index(dir_path))
        widgets.codePageFileTreeView.setAnimated(False)
        widgets.codePageFileTreeView.setIndentation(20)
        widgets.codePageFileTreeView.header().setSectionResizeMode(0, QHeaderView.ResizeToContents)
        widgets.codePageFileTreeView.setSortingEnabled(False)
        widgets.codePageFileTreeView.doubleClicked.connect(self.readFileToCodeBox)
        widgets.btn_home.setStyleSheet(UIFunctions.selectMenu(widgets.btn_home.styleSheet()))

        with open(self.codefileName, "r", encoding='utf-8') as code_file, \
                open(self.errorfilename, "r", encoding='utf-8') as error_file, \
                open("./css/errorText.html", "r", encoding='utf-8') as error_html_file:
            code_text = code_file.read()
            html = highlight(code_text,
                             CLexer(),
                             HtmlFormatter(
                                 linenos="table",
                                 full=True,
                                 cssfile="./css/myStyle.css",
                                 wrapcode=True
                             ))
            widgets.codeText.setHtml(html)
            error_html_template = error_html_file.read()
            for error_text in error_file:
                error_kind = error_text.split(":")[0].lower()
                if error_kind == "info":
                    new_line = f'\t<p class="info">{error_text}</p>\n'
                elif error_kind == "warning":
                    new_line = f'\t<p class="warning">{error_text}</p>\n'
                elif error_kind == "error":
                    new_line = f'\t<p class="error">{error_text}</p>\n'
                error_html_template = error_html_template + new_line
            error_html_template = error_html_template + "</body>\n</html>"
            widgets.errorText.setHtml(error_html_template)

        # SET PicPage
        # ///////////////////////////////////////////////////////////////
        widgets.picKind.activated.connect(self.change_pic_when_pic_combo_refresh)
        widgets.picKind.currentTextChanged.connect(self.change_pic_when_pic_combo_current_file_changed)
        self.ui.picPageView = NewGraphView(self.picfileName, self.ui.picPageView)
        widgets.ASTup.clicked.connect(self.uppic)
        widgets.ASTdown.clicked.connect(self.dowpic)

        # SET ModelPage
        # ///////////////////////////////////////////////////////////////
        widgets.models.clear()
        widgets.models.addItems(self.model_name_list)

        raw_nodes_list = raw_model_struct()
        raw_nodes_list.append("output")
        widgets.model_nodes.clear()
        widgets.model_nodes.addItems(raw_nodes_list)

        widgets.models.currentTextChanged[str].connect(self.change_pic_when_model_combo_selected)
        widgets.model_nodes.currentTextChanged[str].connect(self.change_pic_when_model_nodes_combo_selected)
        self.ui.model_picView = NewGraphView(self.picfileName, self.ui.model_picView)
        self.ui.model_picView.setImage("./pic/models/lenet/net_pic.png")

        global watch_name
        watch_name["didCodeFileChanged"] = self.didCodeFileChanged
        watch_name["codefileName"] = self.codefileName
        watch_name["code_md5"] = self.code_md5
        self.watch_thread = WatchThread()
        self.watch_thread.changed_signal.connect(self.read_changed_code_to_codebox)
        self.watch_thread.start()

    def parse_newpic_with_joern(self, code_file_name, path_to_pic, combo_text):
        tmp_code_filepath = "./tmpFile/tmp_code.cpp"
        shutil.copy(code_file_name, tmp_code_filepath)
        with open(tmp_code_filepath, "r+", encoding="utf-8") as tmp_code:
            code_text = tmp_code.read()
            tmp_code.seek(0)
            tmp_code.truncate()
            cms = rmCommentsInCFile(code_text)
            ms = rm_emptyline(cms)
            s = rm_includeline(ms)
            print(s)
            tmp_code.write(s)
        joern_parse_main_func(tmp_code_filepath, path_to_pic, combo_text)
        self.didCodeFileChanged = False
        watch_name["didCodeFileChanged"] = self.didCodeFileChanged

    def modifysh(self):

        with open('../undefinedvariableshelltest.sh', 'r+', encoding="utf-8") as f:
            result = ''
            for item in f:
                res = re.search(r'clang\+\+ -emit-ast -c [a-zA-Z0-9.: ]+', item)
                if res:
                    print(res.group())
                    tmp = res.group().split(' ')
                    tmp[-1] = 'test.cpp'
                    tmp2 = ' '.join(tmp)
                    result = result + tmp2 + '\n'
                else:
                    result = result + item
            f.seek(0)
            f.truncate()
            f.write(result)
    # ///////////////////////////////////////////////////////////////
    # set code text when code file selected
    # ///////////////////////////////////////////////////////////////
    def render_code(self):
        global widgets
        with open(self.codefileName, "r", encoding='utf-8') as codeFile:
            code_text = codeFile.read()
            self.code_md5 = hashlib.md5(code_text.encode("utf-8")).hexdigest()

            watch_name["didCodeFileChanged"] = self.didCodeFileChanged
            watch_name["codefileName"] = self.codefileName
            watch_name["code_md5"] = self.code_md5

            html = highlight(code_text,
                             CLexer(),
                             HtmlFormatter(
                                 linenos="table",
                                 full=True,
                                 cssfile="./css/myStyle.css",
                                 wrapcode=True
                             ),
                             outfile=None)
            widgets.codeText.setHtml(html)

    def print_error_info(self):
        shutil.copy(self.codefileName, f"../tests/MainChecker/staticApptest.cpp")
        sh_str = "sh ./undefinedvariableshell.sh > staticApp/tmpFile/detect_result.log"
        subprocess.call(sh_str, shell=True, cwd='../')

        with open('tmpFile/detect_result.log', 'r', encoding="utf-8") as f, \
                open("./css/errorText.html", "r", encoding='utf-8') as error_html_file:
            error_html_template = error_html_file.read()
            for item in f:
                res = re.search(r'WARNING:[a-zA-Z0-9\'.: ]+', item)
                if res:
                    print(res.group())
                    error_text = res.group()
                    error_kind = error_text.split(":")[0].lower()
                    #error_text = error_text.title()
                    if error_kind == "info":
                        new_line = f'\t<p class="info">{error_text}</p>\n'
                    elif error_kind == "warning":
                        new_line = f'\t<p class="warning">{error_text}</p>\n'
                    elif error_kind == "error":
                        new_line = f'\t<p class="error">{error_text}</p>\n'
                    error_html_template = error_html_template + new_line
            error_html_template = error_html_template + "</body>\n</html>"
            self.ui.errorText.setHtml(error_html_template)

    def read_changed_code_to_codebox(self):
        self.render_code()
        picview_kind = self.ui.picKind.currentText()
        print(picview_kind + " in [readFileToCodeBox]")
        self.print_error_info()
        # self.parse_newpic_with_joern(self.codefileName, "./tmpFile/tmpCodeForJoern", self.ui.picKind.currentText())
        new_thread = Thread(target=self.parse_newpic_with_joern,
                            args=(
                                self.codefileName, "./tmpFile/tmpCodeForJoern", self.ui.picKind.currentText()))
        new_thread.start()

    def readFileToCodeBox(self, Qmodelidx, code_changed=False):
        """
        Read in the code file and highlight the code according to Qmodelidx
        """
        self.didCodeFileChanged = True
        self.isDefault = False

        watch_name["codefileName"] = self.codefileName
        watch_name["code_md5"] = self.code_md5

        if code_changed:
            self.render_code()
            picview_kind = self.ui.picKind.currentText()
            print(picview_kind + " in [readFileToCodeBox]")
            self.print_error_info()
            self.parse_newpic_with_joern(self.codefileName, "./tmpFile/tmpCodeForJoern", self.ui.picKind.currentText())
        else:
            if self.model.filePath(Qmodelidx).endswith(self.endswith):
                self.codefileName = self.model.filePath(Qmodelidx)
                print(self.codefileName)
                self.render_code()
                picview_kind = self.ui.picKind.currentText()
                print(picview_kind + " in [readFileToCodeBox]")
                if os.path.exists("../tests/MainChecker/cfg") and os.path.exists("../tests/MainChecker/pdg"):
                    shutil.rmtree("../tests/MainChecker/cfg")
                    shutil.rmtree("../tests/MainChecker/pdg")
                new_thread = Thread(target=self.parse_newpic_with_joern,
                                    args=(
                                        self.codefileName, "./tmpFile/tmpCodeForJoern", self.ui.picKind.currentText()))
                new_thread.start()
                self.print_error_info()
            else:
                QMessageBox.warning(self, '格式错误', '选择的文件后缀必须为\n ["c", "cpp", "c++", "h", "hpp"]',
                                    QMessageBox.Ok, QMessageBox.Ok)

    # ///////////////////////////////////////////////////////////////
    # set img when pic combobox text selected
    # ///////////////////////////////////////////////////////////////

    def getFileName(self, path, suffix):
        # 获取指定目录下的所有指定后缀的文件名
        input_template_All = []
        f_list = os.listdir(path)  # 返回文件名
        for i in f_list:
            # os.path.splitext():分离文件名与扩展名
            if os.path.splitext(i)[1] == suffix:
                input_template_All.append((path + i))
                # print(i)
        return sorted(input_template_All)

    def change_pic_when_pic_combo_refresh(self, item_index):
        """
        pic combo
        """
        item_name = self.ui.picKind.itemText(item_index)
        item_name = item_name.lower()
        if self.isDefault:
            png_path = f"./default/defaultpic_{item_name}.png"
            self.ui.picPageView.setImage(png_path)
        else:
            png_path = f"./tmpFile/codeFilePic/{item_name}/code_{item_name}.png"
            self.ui.picPageView.setImage(png_path)
            print(f"{item_name} pic has refreshed! in pic View!")

    def change_pic_when_pic_combo_current_file_changed(self, item_name):
        """
        pic combo changed
        init code struct pic
        """

        item_name = item_name.lower()
        if self.isDefault:
            png_path = f"./default/defaultpic_{item_name}.png"
            self.ui.picPageView.setImage(png_path)
        else:
            if item_name == "ast":
                try:
                    shutil.copy("./tmpFile/tmpCodeForJoern/parse/dot/ast/0-ast.dot",
                                f"./tmpFile/codeFilePic/{item_name}/code_dot.dot")
                    self.astdot = self.getFileName("./tmpFile/tmpCodeForJoern/parse/dot/ast/", ".dot")
                    self.dotindex = 0
                except:
                    pass
            elif item_name == "cdg":
                try:
                    shutil.copy("./tmpFile/tmpCodeForJoern/parse/dot/cdg/1-cdg.dot",
                                f"./tmpFile/codeFilePic/{item_name}/code_dot.dot")
                    self.cdgdot = self.getFileName("./tmpFile/tmpCodeForJoern/parse/dot/cdg/", ".dot")
                    self.dotindex = 0
                except:
                    pass
            elif item_name == "pdg":
                try:
                    shutil.copy("../tests/MainChecker/pdg/main.dot",
                                f"./tmpFile/codeFilePic/{item_name}/code_dot.dot")
                    self.pdgdot = self.getFileName("../tests/MainChecker/pdg/", ".dot")
                    self.dotindex = 0
                except:
                    pass
            elif item_name == "cfg":
                try:

                    shutil.copy("../tests/MainChecker/cfg/main.dot",
                                f"./tmpFile/codeFilePic/{item_name}/code_dot.dot")
                    self.cfgdot = self.getFileName("../tests/MainChecker/cfg/", ".dot")
                    self.dotindex = 0
                except:
                    pass

            shellstr = f"dot -Grankdir=LR -Tpng -o ./tmpFile/codeFilePic/{item_name}/code_{item_name}.png ./tmpFile/codeFilePic/{item_name}/code_dot.dot "
            subprocess.call(shellstr, shell=True)

            png_path = f"./tmpFile/codeFilePic/{item_name}/code_{item_name}.png"
            self.ui.picPageView.setImage(png_path)
            print(f"{item_name} pic has made! in [change_pic_when_pic_combo_current_file_changed]!")

    def uppic(self):
        item_name = self.ui.picKind.currentText().lower()

        if item_name == "ast":
            try:
                length = len(self.astdot)
                self.dotindex = (self.dotindex - 1) % length
                dot_path = self.astdot[self.dotindex]
                shutil.copy(f"{dot_path}",
                            f"./tmpFile/codeFilePic/{item_name}/code_dot.dot")
            except:
                pass
        elif item_name == "cdg":
            try:
                length = len(self.cdgdot)
                self.dotindex = (self.dotindex - 1) % length
                dot_path = self.cdgdot[self.dotindex]
                shutil.copy(f"{dot_path}",
                            f"./tmpFile/codeFilePic/{item_name}/code_dot.dot")
            except:
                pass
        elif item_name == "pdg":
            try:
                length = len(self.pdgdot)
                self.dotindex = (self.dotindex - 1) % length
                dot_path = self.pdgdot[self.dotindex]
                shutil.copy(f"{dot_path}",
                            f"./tmpFile/codeFilePic/{item_name}/code_dot.dot")
            except:
                pass
        elif item_name == "cfg":
            try:
                length = len(self.cfgdot)
                self.dotindex = (self.dotindex - 1) % length
                dot_path = self.cfgdot[self.dotindex]
                shutil.copy(f"{dot_path}",
                            f"./tmpFile/codeFilePic/{item_name}/code_dot.dot")
            except:
                pass
        shellstr = f"dot -Grankdir=LR -Tpng -o ./tmpFile/codeFilePic/{item_name}/code_{item_name}.png ./tmpFile/codeFilePic/{item_name}/code_dot.dot "
        subprocess.call(shellstr, shell=True)

        png_path = f"./tmpFile/codeFilePic/{item_name}/code_{item_name}.png"
        self.ui.picPageView.setImage(png_path)
        print(f"{item_name} pic has made! in [uppic]!")

    def dowpic(self):
        item_name = self.ui.picKind.currentText().lower()

        if item_name == "ast":
            try:
                length = len(self.astdot)
                self.dotindex = (self.dotindex + 1) % length
                dot_path = self.astdot[self.dotindex]
                shutil.copy(f"{dot_path}",
                            f"./tmpFile/codeFilePic/{item_name}/code_dot.dot")
            except:
                pass
        elif item_name == "cdg":
            try:
                length = len(self.cdgdot)
                self.dotindex = (self.dotindex + 1) % length
                dot_path = self.cdgdot[self.dotindex]
                shutil.copy(f"{dot_path}",
                            f"./tmpFile/codeFilePic/{item_name}/code_dot.dot")
            except:
                pass
        elif item_name == "pdg":
            try:
                length = len(self.pdgdot)
                self.dotindex = (self.dotindex + 1) % length
                dot_path = self.pdgdot[self.dotindex]
                shutil.copy(f"{dot_path}",
                            f"./tmpFile/codeFilePic/{item_name}/code_dot.dot")
            except:
                pass
        elif item_name == "cfg":
            try:
                length = len(self.cfgdot)
                self.dotindex = (self.dotindex + 1) % length
                dot_path = self.cfgdot[self.dotindex]
                shutil.copy(f"{dot_path}",
                            f"./tmpFile/codeFilePic/{item_name}/code_dot.dot")
            except:
                pass

        shellstr = f"dot -Grankdir=LR -Tpng -o ./tmpFile/codeFilePic/{item_name}/code_{item_name}.png ./tmpFile/codeFilePic/{item_name}/code_dot.dot "
        subprocess.call(shellstr, shell=True)

        png_path = f"./tmpFile/codeFilePic/{item_name}/code_{item_name}.png"
        self.ui.picPageView.setImage(png_path)
        print(f"{item_name} pic has made! in [dowpic]!")

    # ///////////////////////////////////////////////////////////////
    # set img when pic combobox text selected
    # ///////////////////////////////////////////////////////////////

    def change_pic_when_model_combo_selected(self, item_name):
        """
        model combo
        when model changed
        """
        raw_nodes_list = raw_model_struct(item_name)
        self.ui.model_nodes.clear()
        raw_nodes_list.append("output")
        self.ui.model_nodes.addItems(raw_nodes_list)
        self.ui.model_picView.setImage(f"./pic/models/{item_name}/net_pic.png")

    def change_pic_when_model_nodes_combo_selected(self, item_name):
        """
        model_nodes combo
        when nodes changed
        """
        if not item_name:
            return
        if item_name != "output":
            get_node_graph(self.ui.models.currentText(), item_name)
            self.ui.model_picView.setImage(f"./tmpFile/tmpPic.png")
        else:
            self.ui.model_picView.setImage(f"./pic/models/{self.ui.models.currentText()}/net_pic.png")

    def wheelEvent(self, e: QWheelEvent):
        """
        send wheel event to new pic view
        """
        if self.pic_or_model == "pic":
            self.ui.picPageView.wheelEvent(e)
        elif self.pic_or_model == "model":
            self.ui.model_picView.wheelEvent(e)

    def resizeEvent(self, e):
        """
        send resize event to pic view
        """
        UIFunctions.resize_grips(self)
        if self.pic_or_model == "pic":
            self.ui.picPageView.resizeEvent(e)
        elif self.pic_or_model == "model":
            self.ui.model_picView.resizeEvent(e)

    # ///////////////////////////////////////////////////////////////
    # BUTTONS CLICK
    # ///////////////////////////////////////////////////////////////
    def buttonClick(self):
        """
        a set func of button clicked
        """
        # GET BUTTON CLICKED
        btn = self.sender()
        btnName = btn.objectName()

        # SHOW HOME PAGE
        if btnName == "btn_home":
            self.pic_or_model = "home"
            widgets.stackedWidget.setCurrentWidget(widgets.home)
            UIFunctions.resetStyle(self, btnName)
            btn.setStyleSheet(UIFunctions.selectMenu(btn.styleSheet()))

        # SHOW MODEL PAGE
        if btnName == "btn_model":
            self.pic_or_model = "model"
            self.ui.model_picView.setImage("./pic/models/lenet/net_pic.png")
            widgets.stackedWidget.setCurrentWidget(widgets.model_page)
            UIFunctions.resetStyle(self, btnName)
            btn.setStyleSheet(UIFunctions.selectMenu(btn.styleSheet()))

        # SHOW PIC SHOW PAGE
        if btnName == "btn_picShow":
            self.pic_or_model = "pic"
            self.change_pic_when_pic_combo_current_file_changed(self.ui.picKind.currentText())
            widgets.stackedWidget.setCurrentWidget(widgets.pic_page)  # SET PAGE
            UIFunctions.resetStyle(self, btnName)  # RESET ANOTHERS BUTTONS SELECTED
            btn.setStyleSheet(UIFunctions.selectMenu(btn.styleSheet()))  # SELECT MENU

        if btnName == "btn_save":
            print("Save BTN clicked!")

        # PRINT BTN NAME
        print(f'Button "{btnName}" pressed!')

    # MOUSE CLICK EVENTS
    # ///////////////////////////////////////////////////////////////
    def mousePressEvent(self, event):
        """
        a set method when mouse press
        """
        # SET DRAG POS WINDOW
        self.dragPos = event.globalPosition().toPoint()

        # PRINT MOUSE EVENTS
        if event.buttons() == Qt.LeftButton:
            print('Mouse click: LEFT CLICK')
        if event.buttons() == Qt.RightButton:
            print('Mouse click: RIGHT CLICK')


class NewGraphView(QGraphicsView):
    """ 图片查看器 """

    def __init__(self, pic_path: str, myPicPageView: QGraphicsView, parent=None):
        super().__init__(parent=parent)
        self.zoomInTimes = 0
        self.maxZoomInTimes = 25

        # 创建场景
        self.graphicsScene = QGraphicsScene()

        # 图片
        self.pixmap = QPixmap(pic_path)
        if not self.pixmap:
            print("no pixmap in [NewGraphView]")
        self.pixmapItem = QGraphicsPixmapItem(self.pixmap)
        self.displayedImageSize = QSize(0, 0)

        self.picView = myPicPageView
        # 初始化小部件
        self.__initWidget()

    def __initWidget(self):

        self.picView.setVerticalScrollBarPolicy(Qt.ScrollBarAlwaysOff)
        self.picView.setHorizontalScrollBarPolicy(Qt.ScrollBarAlwaysOff)

        # 以鼠标所在位置为锚点进行缩放
        self.picView.setTransformationAnchor(QGraphicsView.AnchorUnderMouse)

        # 平滑缩放
        self.pixmapItem.setTransformationMode(Qt.SmoothTransformation)
        self.picView.setRenderHints(QPainter.Antialiasing |
                                    QPainter.SmoothPixmapTransform)

        # 设置场景
        self.graphicsScene.addItem(self.pixmapItem)
        self.picView.setScene(self.graphicsScene)

    def wheelEvent(self, e: QWheelEvent):
        """ 滚动鼠标滚轮缩放图片 """
        if e.angleDelta().y() > 0:
            self.zoomIn()
        else:
            self.zoomOut()

    def resizeEvent(self, e):
        """ 缩放图片 """
        self.picView.resizeEvent(e)

        if self.zoomInTimes > 0:
            return

        # 调整图片大小
        ratio = self.__getScaleRatio()
        self.displayedImageSize = self.pixmap.size() * ratio
        if ratio < 1:
            self.fitInView(self.pixmapItem, Qt.KeepAspectRatio)
        else:
            self.resetTransform()

    def setImage(self, imagePath: str):
        """ 设置显示的图片 """
        self.resetTransform()

        # 刷新图片
        self.pixmap = QPixmap(imagePath)
        self.pixmapItem.setPixmap(self.pixmap)

        # 调整图片大小
        self.picView.setSceneRect(QRectF(self.pixmap.rect()))
        ratio = self.__getScaleRatio()
        self.displayedImageSize = self.pixmap.size() * ratio
        if ratio < 1:
            self.fitInView(self.pixmapItem, Qt.KeepAspectRatio)

    def resetTransform(self):
        """ 重置变换 """
        self.picView.resetTransform()
        self.zoomInTimes = 0
        self.__setDragEnabled(False)

    def __isEnableDrag(self):
        """ 根据图片的尺寸决定是否启动拖拽功能 """
        v = self.picView.verticalScrollBar().maximum() > 0
        h = self.picView.horizontalScrollBar().maximum() > 0
        return v or h

    def __setDragEnabled(self, isEnabled: bool):
        """ 设置拖拽是否启动 """
        self.picView.setDragMode(
            self.picView.ScrollHandDrag if isEnabled else self.picView.NoDrag)

    def __getScaleRatio(self):
        """ 获取显示的图像和原始图像的缩放比例 """
        if self.pixmap.isNull():
            return 1

        pw = self.pixmap.width()
        ph = self.pixmap.height()
        rw = min(1, self.width() / pw)
        rh = min(1, self.height() / ph)
        return min(rw, rh)

    def fitInView(self, item: QGraphicsItem, mode=Qt.KeepAspectRatio):
        """ 缩放场景使其适应窗口大小 """
        self.picView.fitInView(item, mode)
        self.displayedImageSize = self.__getScaleRatio() * self.pixmap.size()
        self.zoomInTimes = 0

    def zoomIn(self, viewAnchor=QGraphicsView.AnchorUnderMouse):
        """ 放大图像 """
        if self.zoomInTimes == self.maxZoomInTimes:
            return

        self.picView.setTransformationAnchor(viewAnchor)

        self.zoomInTimes += 1
        self.picView.scale(1.1, 1.1)
        self.__setDragEnabled(self.__isEnableDrag())

        # 还原 anchor
        self.picView.setTransformationAnchor(QGraphicsView.AnchorUnderMouse)

    def zoomOut(self, viewAnchor=QGraphicsView.AnchorUnderMouse):
        """ 缩小图像 """
        if self.zoomInTimes == 0 and not self.__isEnableDrag():
            return

        self.picView.setTransformationAnchor(viewAnchor)

        self.zoomInTimes -= 1

        # 原始图像的大小
        pw = self.pixmap.width()
        ph = self.pixmap.height()

        # 实际显示的图像宽度
        w = self.displayedImageSize.width() * 1.1 ** self.zoomInTimes
        h = self.displayedImageSize.height() * 1.1 ** self.zoomInTimes

        if pw > self.picView.width() or ph > self.picView.height():
            # 在窗口尺寸小于原始图像时禁止继续缩小图像比窗口还小
            if w <= self.picView.width() and h <= self.picView.height():
                self.fitInView(self.pixmapItem)
            else:
                self.picView.scale(1 / 1.1, 1 / 1.1)
        else:
            # 在窗口尺寸大于图像时不允许缩小的比原始图像小
            if w <= pw:
                self.resetTransform()
            else:
                self.picView.scale(1 / 1.1, 1 / 1.1)

        self.__setDragEnabled(self.__isEnableDrag())

        # 还原 anchor
        self.picView.setTransformationAnchor(QGraphicsView.AnchorUnderMouse)


def check_environment():
    """
    check environment. If not success in some scene, error raise.
    """
    if os.path.isdir("./images/icons") and os.path.isdir("./images/images"):
        print("images and icons folder exists!")
    else:
        logging.error("images and icons folder does not exist!")

    if os.path.isfile("./css/myStyle.css"):
        print("css file exists")
    else:
        logging.error("css file does not exist!")

    if os.path.isfile("./default/default.c") \
            and os.path.isfile("./default/defaultpic_ast.png") \
            and os.path.isfile("./default/welcome_default.png"):
        print("default folder exists!")
    else:
        logging.error("default folder does not exist!")


# 判断dictSymbols的key中，最先出现的符号是哪个，并返回其所在位置以及该符号
def get1stSymPos(s, fromPos=0):
    # 清除注释用，对能干扰清除注释的东西，进行判断
    g_DictSymbols = {'"': '"', '/*': '*/', '//': '\n'}
    listPos = []  # 位置,符号
    for b in g_DictSymbols:
        pos = s.find(b, fromPos)
        listPos.append((pos, b))  # 插入位置以及结束符号
    minIndex = -1  # 最小位置在listPos中的索引
    index = 0  # 索引
    while index < len(listPos):
        pos = listPos[index][0]  # 位置
        if minIndex < 0 and pos >= 0:  # 第一个非负位置
            minIndex = index
        if 0 <= pos < listPos[minIndex][0]:  # 后面出现的更靠前的位置
            minIndex = index
        index = index + 1
    if minIndex == -1:  # 没找到
        return (-1, None)
    else:
        return (listPos[minIndex])


# 去掉cpp文件的注释
def rmCommentsInCFile(s):
    # 全局变量，清除注释用，对能干扰清除注释的东西，进行判断
    g_DictSymbols = {'"': '"', '/*': '*/', '//': '\n'}
    if not isinstance(s, str):
        raise TypeError(s)
    fromPos = 0
    while (fromPos < len(s)):
        result = get1stSymPos(s, fromPos)
        logging.info(result)
        if result[0] == -1:  # 没有符号了
            return s
        else:
            endPos = s.find(g_DictSymbols[result[1]], result[0] + len(result[1]))
            if result[1] == '//':  # 单行注释
                if endPos == -1:  # 没有换行符也可以
                    endPos = len(s)
                s = s.replace(s[result[0]:endPos], '', 1)
                fromPos = result[0]
            elif result[1] == '/*':  # 区块注释
                if endPos == -1:  # 没有结束符就报错
                    raise ValueError("块状注释未闭合")
                s = s.replace(s[result[0]:endPos + 2], '', 1)
                fromPos = result[0]
            else:  # 字符串
                if endPos == -1:  # 没有结束符就报错
                    raise ValueError("符号未闭合")
                fromPos = endPos + len(g_DictSymbols[result[1]])
    return s


# 去除程序中的空行
def rm_emptyline(ms):
    if not isinstance(ms, str):
        raise TypeError(ms)
    ms = "".join([s for s in ms.splitlines(True) if s.strip()])
    return ms


# 去除程序中的头文件
def rm_includeline(ms):
    if not isinstance(ms, str):
        raise TypeError(ms)
    ms = "".join([s for s in ms.splitlines(True) if '#include' not in s])
    return ms


if __name__ == "__main__":
    check_environment()
    app = QApplication(sys.argv)
    window = MainWindow()
    app.setWindowIcon(QIcon("./images/images/birds.ico"))
    sys.exit(app.exec())
