import os
import shutil
import subprocess

JOERNPATH = "./third_party/joern_files/joern/joern-cli/"


def parse_source_code_to_dot(file_path: str, f: str, root_out_dir: str):
    """
        parse_source_code_to_dot将文件解析成dot并存储进对应目录
        Args:
            file_path: 需要解析的文件的路径
            f: file_path
            root_out_dir: 输出dot的路径
    """

    out_dir_ast = root_out_dir + "/parse/dot/ast"
    out_dir_cdg = root_out_dir + "/parse/dot/cdg"
    out_dir_cpg = root_out_dir + "/parse/dot/cpg"
    out_dir_pdg = root_out_dir + "/parse/dot/pdg"

    try:
        os.makedirs(out_dir_ast)
        os.makedirs(out_dir_cdg)
        os.makedirs(out_dir_cpg)
        os.makedirs(out_dir_pdg)
    except:
        pass
    # parse source code into cpg
    print('parseing source code into dir...')
    shell_str = "sh " + JOERNPATH + "./joern-parse " + file_path
    subprocess.call(shell_str, shell=True)
    print('exporting dot from dir...')
    # 导出cpg的dot文件到指定的文件夹中
    # 本处指定的是：out_dir_cpg + fname
    # 即：'./data/parsed/dot/cpg/'

    shellrmDir = "rm -r " + root_out_dir + "/parse/dot/*"
    subprocess.call(shellrmDir, shell=True)

    shell_export_ast = "sh " + JOERNPATH + "joern-export " + "--repr ast --out " + out_dir_ast + os.sep
    subprocess.call(shell_export_ast, shell=True)

    # shell_export_cpg = "sh " + JOERNPATH + "joern-export " + "--repr cpg14 --out " + out_dir_cpg + f.split('..')[
    #     0] + os.sep
    # subprocess.call(shell_export_cpg, shell=True)

    shell_export_cdg = "sh " + JOERNPATH + "joern-export " + "--repr cdg --out " + out_dir_cdg + os.sep
    subprocess.call(shell_export_cdg, shell=True)

    # shell_export_pdg = "sh " + JOERNPATH + "joern-export " + "--repr pdg --out " + out_dir_pdg + f.split('..')[
    #     0] + os.sep
    # subprocess.call(shell_export_pdg, shell=True)


def joern_parse_main_func(source_code_path: str = "./default/default.c",
                          out_dir: str = "./tmpFile/tmpCodeForJoern",
                          item_name: str = "ast"):
    """
    main_func函数可解析特定C++文件
    Args:
        source_code_path: 需要解析的文件的路径
        out_dir: 输出dot的路径
    """
    item_name = item_name.lower()
    print(f'----starting to process {source_code_path} with joern-----')
    parse_source_code_to_dot(file_path=source_code_path,
                             f=source_code_path,
                             root_out_dir=out_dir)

    if item_name == "ast":
        try:
            shutil.copy(f"{out_dir}/parse/dot/ast/0-ast.dot",
                        f"./tmpFile/codeFilePic/{item_name}/code_dot.dot")
        except:
            pass
    elif item_name == "cdg":
        try:
            shutil.copy(f"{out_dir}/parse/dot/cdg/1-cdg.dot",
                        f"./tmpFile/codeFilePic/{item_name}/code_dot.dot")
        except:
            pass

    shellstr = f"dot -Grankdir=LR -Tpng -o ./tmpFile/codeFilePic/{item_name}/code_{item_name}.png ./tmpFile/codeFilePic/{item_name}/code_dot.dot "
    subprocess.call(shellstr, shell=True)
