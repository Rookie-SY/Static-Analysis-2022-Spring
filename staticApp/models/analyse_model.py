import json
import subprocess


def getpath_from_node(dot_filepath, node_label):
    """
    根据用户选择的需要可视化的节点，从原dot中提取子图并展示
    Args:
        dot_filepath: 源dot路径
        node_label: 想要查看的可到达节点的label

    Returns:
        展示提取的子图
    """

    # 将源dot文件转成json格式方便后续操作
    shellstr = "dot -Tdot_json -o ./tmpFile/tmpJson.json " + dot_filepath
    subprocess.call(shellstr, shell=True)

    # raw_data 存储源dot文件转化的json信息
    # MyNewData 存储最终所需的json信息
    my_new_dot = {}
    with open("./tmpFile/tmpJson.json", "r", encoding="utf-8") as f:
        raw_data = json.load(f)

    for item in raw_data:
        if not (item == 'objects' or item == 'edges'):
            my_new_dot[item] = raw_data[item]

    target_node = {'objects': [], 'edges': []}
    node_gvid = []
    for item in raw_data['objects']:
        if item['label'] == node_label:
            target_node['objects'].append(item)
            node_gvid.append(item['_gvid'])

    # print(target_node)
    # print(node_gvid)

    while node_gvid:
        gvid = node_gvid[0]

        for edgesitem in raw_data['edges']:
            if edgesitem['head'] == gvid:
                node_gvid.append(edgesitem['tail'])
                target_node['edges'].append(edgesitem)

                for objectsitem in raw_data['objects']:
                    if objectsitem['_gvid'] == edgesitem['tail']:
                        target_node['objects'].append(objectsitem)

        node_gvid.pop(0)
    # print(target_node)

    my_new_dot.update(target_node)
    if my_new_dot == raw_data:
        print("yes")
    # print(my_new_dot)

    name_dict = {}

    # 生成新的dot文件
    new_graph_head = """digraph {
    """

    graph_param = f"\tgraph [size = \"{my_new_dot['size']}\"]\n"

    tmp_obj = my_new_dot['objects'][0]
    node_param = f"\tnode [align={tmp_obj['align']} fontname={tmp_obj['fontname']} fontsize={tmp_obj['fontsize']} height={tmp_obj['height']} ranksep={tmp_obj['ranksep']} shape={tmp_obj['shape']} style={tmp_obj['style']}]\n "

    # print(new_graph_head + graph_param + node_param)

    graph_param = new_graph_head + graph_param + node_param

    for objitem in my_new_dot['objects']:
        if "fillcolor" in objitem.keys():
            new_node = f"\t{objitem['name']} [label=\"{objitem['label']}\" fillcolor={objitem['fillcolor']}]\n"
            name_dict[objitem['_gvid']] = objitem['name']
        else:
            new_node = f"\t{objitem['name']} [label=\"{objitem['label']}\"]\n"
            name_dict[objitem['_gvid']] = objitem['name']
        graph_param = graph_param + new_node

    for edgesitem in my_new_dot['edges']:
        in_node = edgesitem['tail']
        out_node = edgesitem['head']

        new_node = f"\t{name_dict[in_node]} -> {name_dict[out_node]}\n"
        graph_param = graph_param + new_node

    graph_param = graph_param + "}"

    # print(graph_param)

    with open("tmpFileForDot.dot", "w", encoding="utf-8") as f:
        f.write(graph_param)


getpath_from_node("./data/NetDot", "2699565535392 ConvolutionBackward0")
