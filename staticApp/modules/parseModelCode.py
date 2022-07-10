import torch
import json
import subprocess
from models import LeNet
from models import SqueezeNet
from models import ConvNet
from models import make_dot
from models import getpath_from_node
from torchvision.models import convnext_tiny
from torchvision.models import convnext_small
from torchvision.models import convnext_base
from torchvision.models import convnext_large
from torchvision.models import densenet121
from torchvision.models import alexnet
from torchvision.models import googlenet

__all__ = ['raw_model_struct', 'get_node_graph']


def raw_model_struct(model_name: str = "lenet") -> list:
    """获得指定模型的计算图
        Args:
        model_name: model名称

    Returns:
        node_name_list: model所有节点的名字
    """
    if model_name == "lenet":
        x = torch.randn(1, 1, 28, 28)  # 定义一个网络的输入值
        model = LeNet()
    elif model_name == "squeezenet":
        x = torch.randn(1, 3, 224, 224)  # 定义一个网络的输入值
        model = SqueezeNet()
    # elif model_name == "convnext_tiny":
    #     x = torch.randn(1, 3, 224, 224)  # 定义一个网络的输入值
    #     model = convnext_tiny()
    # elif model_name == "convnext_small":
    #     x = torch.randn(1, 3, 224, 224)  # 定义一个网络的输入值
    #     model = convnext_small()
    # elif model_name == "convnext_base":
    #     x = torch.randn(1, 3, 384, 384)  # 定义一个网络的输入值
    #     model = convnext_base()
    # elif model_name == "convnext_large":
    #     x = torch.randn(1, 3, 384, 384)  # 定义一个网络的输入值
    #     model = convnext_large()
    # elif model_name == "densenet121":
    #     x = torch.randn(1, 3, 224, 224)  # 定义一个网络的输入值
    #     model = densenet121()
    # elif model_name == "alexnet":
    #     x = torch.randn(1, 3, 224, 224)  # 定义一个网络的输入值
    #     model = alexnet()
    elif model_name == "googlenet":
        x = torch.randn(1, 3, 224, 224)  # 定义一个网络的输入值
        model = googlenet()
    else:
        x = torch.randn(1, 1, 28, 28)  # 定义一个网络的输入值
        model = ConvNet()

    y = model(x)
    MyNetVis = make_dot(y, params=dict(list(model.named_parameters()) + [('x', x)]))
    # 指定文件生成的文件夹
    MyNetVis.directory = f"./pic/models/{model_name}"
    MyNetVis.save("net_dot.dot")

    shellstr = f"dot -Tpng -o ./pic/models/{model_name}/net_pic.png ./pic/models/{model_name}/net_dot.dot"
    subprocess.call(shellstr, shell=True)

    shellstr = f"dot -Tdot_json -o ./pic/models/{model_name}/net_json.json ./pic/models/{model_name}/net_dot.dot"
    subprocess.call(shellstr, shell=True)

    node_name_list = []
    with open(f"./pic/models/{model_name}/net_json.json", "r", encoding="utf-8") as f:
        raw_data = json.load(f)
        for item in raw_data['objects']:
            new_id= item['label'].split(" ")[0]
            if new_id.isdigit():
                node_name_list.append(item['label'])
        node_name_list = sorted(node_name_list, key=lambda k: k.split(" ")[1])
        return node_name_list


def get_node_graph(model_name: str = "lenet", node_name: str = "1 ConvolutionBackward0"):
    """
    获得指定节点的子图
    Args:
        model_name: model名称
        node_name: node名称
    """
    getpath_from_node(f"./pic/models/{model_name}/net_dot.dot", node_name)
    shellstr = f"dot -Tpng -o ./tmpFile/tmpPic.png ./tmpFile/tmpFileForDot.dot"
    subprocess.call(shellstr, shell=True)
