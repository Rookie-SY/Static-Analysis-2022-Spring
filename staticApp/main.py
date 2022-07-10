import sys
import os
import platform

# IMPORT / GUI AND MODULES AND WIDGETS
# ///////////////////////////////////////////////////////////////
from PySide6 import QtCore

from modules import *
from widgets import *
from pygments import highlight
from pygments.lexers import CppLexer, CLexer
from pygments.lexers import get_lexer_by_name
from pygments.formatters import HtmlFormatter

os.environ["QT_FONT_DPI"] = "128"  # FIX Problem for High DPI and Scale above 100%

# SET AS GLOBAL WIDGETS
# ///////////////////////////////////////////////////////////////
widgets = None

class MainWindow(QMainWindow):
    def __init__(self):
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
        self.codefileName = "./default/default.c"

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
            UIFunctions.toggleLeftBox(self, True)

        widgets.toggleLeftBox.clicked.connect(openCloseLeftBox)
        widgets.extraCloseColumnBtn.clicked.connect(openCloseLeftBox)

        # EXTRA RIGHT BOX
        def openCloseRightBox():
            UIFunctions.toggleRightBox(self, False)
        # widgets.settingsTopBtn.clicked.connect(openCloseRightBox)

        # SHOW APP
        # ///////////////////////////////////////////////////////////////
        self.show()

        # SET CUSTOM THEME
        # ///////////////////////////////////////////////////////////////
        useCustomTheme = True
        themeFile = "themes/py_dracula_light.qss"

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

        with open(self.codefileName, "r", encoding='utf-8') as codeFile:
            code_text = codeFile.read()
            html = highlight(code_text,
                             CLexer(),
                             HtmlFormatter(
                                 linenos="table",
                                 full=True,
                                 cssfile="./css/myStyle.css",
                                 wrapcode=True
                             ))
            widgets.codeText.setHtml(html)

        self.pic_or_model = "pic"

        # SET PicPage
        # ///////////////////////////////////////////////////////////////
        widgets.picKind.currentTextChanged[str].connect(self.change_pic_when_pic_combo_selected)
        self.ui.picPageView = NewGraphView(self.picfileName, self.ui.picPageView)

        # SET ModelPage
        # ///////////////////////////////////////////////////////////////
        widgets.model_nodes.currentTextChanged[str].connect(self.change_pic_when_model_combo_selected)
        self.ui.model_picView = NewGraphView(self.picfileName, self.ui.model_picView)

    # ///////////////////////////////////////////////////////////////
    # set code text when code file selected
    # ///////////////////////////////////////////////////////////////
    def readFileToCodeBox(self, Qmodelidx):
        if self.model.filePath(Qmodelidx).endswith(self.endswith):
            self.codefileName = self.model.filePath(Qmodelidx)
            print(self.codefileName)
            with open(self.codefileName, "r", encoding='utf-8') as codeFile:
                code_text = codeFile.read()
                html = highlight(code_text,
                                 CLexer(),
                                 HtmlFormatter(
                                     linenos="table",
                                     full=True,
                                     cssfile="./css/myStyle.css",
                                     wrapcode=True
                                 ))
                # with open("./testFile/testcodeHtml.html","w+",encoding='utf-8') as codeHtml:
                # codeHtml.write(html)
                widgets.codeText.setHtml(html)
            picview_kind = self.ui.picKind.currentText()
            print(picview_kind + " in [readFileToCodeBox]")
            self.change_pic_when_combo_selected(picview_kind)

    # ///////////////////////////////////////////////////////////////
    # set img when pic combobox text selected
    # ///////////////////////////////////////////////////////////////
    def change_pic_when_pic_combo_selected(self, item_name):
        if item_name == "AST":
            print("ast selected")
            self.ui.picPageView.setImage("./default/defaultpic_ast.png")
        elif item_name == "CFG":
            print("cfg selected")
            self.ui.picPageView.setImage("./default/defaultpic_cfg.png")
        elif item_name == "CDG":
            print("cdg selected")
            self.ui.picPageView.setImage("./default/defaultpic_cdg.png")
        elif item_name == "PDG":
            print("pdg selected")
            self.ui.picPageView.setImage("./default/defaultpic_pdg.png")
        else:
            print("some wrong item_name")

    # ///////////////////////////////////////////////////////////////
    # set img when pic combobox text selected
    # ///////////////////////////////////////////////////////////////

    def change_pic_when_model_combo_selected(self, item_name):
        # TODO:
        # change filename
        if item_name == "AST":
            print("ast selected")
            self.ui.model_picView.setImage("./default/defaultpic_ast.png")
        elif item_name == "CFG":
            print("cfg selected")
            self.ui.model_picView.setImage("./default/defaultpic_cfg.png")
        elif item_name == "CDG":
            print("cdg selected")
            self.ui.model_picView.setImage("./default/defaultpic_cdg.png")
        elif item_name == "PDG":
            print("pdg selected")
            self.ui.model_picView.setImage("./default/defaultpic_pdg.png")
        else:
            print("some wrong item_name")

    def wheelEvent(self, e: QWheelEvent):
        if self.pic_or_model == "pic":
            self.ui.picPageView.wheelEvent(e)
        elif self.pic_or_model == "model":
            self.ui.model_picView.wheelEvent(e)

    def resizeEvent(self, e):
        if self.pic_or_model == "pic":
            self.ui.picPageView.resizeEvent(e)
        elif self.pic_or_model == "model":
            self.ui.model_picView.resizeEvent(e)

    # ///////////////////////////////////////////////////////////////
    # BUTTONS CLICK
    # Post here your functions for clicked buttons
    # ///////////////////////////////////////////////////////////////
    def buttonClick(self):
        # GET BUTTON CLICKED
        btn = self.sender()
        btnName = btn.objectName()

        # SHOW HOME PAGE
        if btnName == "btn_home":
            widgets.stackedWidget.setCurrentWidget(widgets.home)
            UIFunctions.resetStyle(self, btnName)
            btn.setStyleSheet(UIFunctions.selectMenu(btn.styleSheet()))

        # SHOW WIDGETS PAGE
        if btnName == "btn_model":
            self.pic_or_model = "model"
            widgets.stackedWidget.setCurrentWidget(widgets.model_page)
            UIFunctions.resetStyle(self, btnName)
            btn.setStyleSheet(UIFunctions.selectMenu(btn.styleSheet()))

        # SHOW NEW PAGE
        if btnName == "btn_picShow":
            self.pic_or_model = "pic"
            widgets.stackedWidget.setCurrentWidget(widgets.pic_page)  # SET PAGE
            UIFunctions.resetStyle(self, btnName)  # RESET ANOTHERS BUTTONS SELECTED
            btn.setStyleSheet(UIFunctions.selectMenu(btn.styleSheet()))  # SELECT MENU

        if btnName == "btn_save":
            print("Save BTN clicked!")

        # PRINT BTN NAME
        print(f'Button "{btnName}" pressed!')

    # RESIZE EVENTS
    # ///////////////////////////////////////////////////////////////
    def resizeEvent(self, event):
        # Update Size Grips
        UIFunctions.resize_grips(self)

    # MOUSE CLICK EVENTS
    # ///////////////////////////////////////////////////////////////
    def mousePressEvent(self, event):
        # SET DRAG POS WINDOW
        self.dragPos = event.globalPos()

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
        self.maxZoomInTimes = 22

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


if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = MainWindow()
    app.setWindowIcon(QIcon("./images/images/birds.ico"))
    sys.exit(app.exec())
