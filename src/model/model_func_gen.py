# Usage:
#   Uncomment/write the generate block for a specific function at the bottom of the script, then in command line console (on Windows):
#       python model_func_gen.py > output.txt

FLAG_CONST          = 1
FLAG_TEMPLATE       = 2
FLAG_ARRAY_ARGS     = 4
FLAG_ARRAY_COMMENTS = 8
FLAG_GETITEM_REPORT = 16

class ExtraArg:
    def __init__(self, _type, _name, func_code = None, default_val = None):
        self.type        = _type
        self.name        = _name
        self.func_code   = func_code if func_code else _name
        self.default_val = '' if default_val is None else f' = {default_val}'

class ExtraTemplateType:
    def __init__(self, _type, _func_call):
        self.type      = _type
        self.func_call = _func_call

    def replace_type(self, type_str):
        twords = []
        cur_word = ''
        for ch in type_str:
            if ch.isalnum():
                cur_word += ch
            else:
                if cur_word:
                    twords.append(cur_word)
                    cur_word = ''
                twords.append(ch)
        if cur_word:
            twords.append(cur_word)

        result = ''
        for w in twords:
            result += self.type if w == 'T' else w
        return result

def generate(class_name, func_name, return_type, comment, flags = 0, func_call = None, extra_args = None, extra_template_types = None):

    constMod = ' const' if (flags & FLAG_CONST) else ''
    constNifItemPtrType = 'const NifItem *'
    returnOp = '' if return_type == 'void' else 'return '
    funcCallName = func_call if func_call else func_name
    templatePrefix = 'template <typename T> ' if (flags & FLAG_TEMPLATE) else ''

    extraArgDeclare1 = ''
    extraArgDeclare2 = ''
    extraArgList = ''
    if extra_args:
        if type(extra_args) is ExtraArg:
            extra_args = [extra_args, ]
        for xarg in extra_args:
            extraArgDeclare1 += f', {xarg.type} {xarg.name}{xarg.default_val}'
            extraArgDeclare2 += f', {xarg.type} {xarg.name}'
            extraArgList += f', {xarg.func_code}'
    else:
        extra_args = []

    if (flags & FLAG_TEMPLATE) and extra_template_types:
        if type(extra_template_types) is ExtraTemplateType:
            extra_template_types = [extra_template_types, ]
        for xt in extra_template_types:
            xt.return_type = xt.replace_type(return_type)
            xt.arg_declare = ''
            for xarg in extra_args:
                local_xarg_type = xt.replace_type(xarg.type)
                xt.arg_declare += f', {local_xarg_type} {xarg.name}'
    else:
        extra_template_types = []


    openBracket = '{'
    closeBracket = '}'

    if not comment.endswith('.'):
        comment += '.'

    if (flags & FLAG_ARRAY_COMMENTS):
        comment_child = comment.replace(' an array', ' a child array')
        comment_index = comment.replace(' an array', ' a model index array')
    else:
        comment_child = comment.replace(' an item', ' a child item')
        comment_index = comment.replace(' an item', ' a model index')

    if (flags & FLAG_ARRAY_ARGS):
        itemArgName        = 'arrayRootItem'
        indexArgName       = 'iArray'
        itemParentArgName  = 'arrayParent'
        indexParentArgName = itemParentArgName
        itemIndexArgName   = 'arrayIndex'
        itemNameArgName    = 'arrayName'
    else:
        itemArgName        = 'item'
        indexArgName       = 'index'
        itemParentArgName  = 'itemParent'
        indexParentArgName = itemParentArgName
        itemIndexArgName   = 'itemIndex'
        itemNameArgName    = 'itemName'

    if (flags & FLAG_GETITEM_REPORT):
        getItemReportArg = ', true'
    else:
        getItemReportArg = ''

    indent = ''
    func_bodies = []

    def func_declare(base_args, get_item_code, func_comment):
        if func_comment:
            comment_prefix = '!'
            for comment_ln in func_comment.split('\n'):
                print(f'{indent}//{comment_prefix} {comment_ln}')
                comment_prefix = ''
        print(f'{indent}{templatePrefix}{return_type} {func_name}( {base_args}{extraArgDeclare1} ){constMod};')
        if get_item_code:
            func_bodies.append((base_args, get_item_code))

    indent = '\t'
    localPtrType = constNifItemPtrType if (flags & FLAG_CONST) else 'NifItem *'
    func_declare(f'{localPtrType} {itemArgName}', f'{itemArgName}' if funcCallName != func_name else None, comment)
    # print(f'{indent}')
    func_declare(f'{constNifItemPtrType} {itemParentArgName}, int {itemIndexArgName}', f'getItem({itemParentArgName}, {itemIndexArgName}{getItemReportArg})', comment_child)
    func_declare(f'{constNifItemPtrType} {itemParentArgName}, const QString & {itemNameArgName}', f'getItem({itemParentArgName}, {itemNameArgName}{getItemReportArg})', comment_child)
    func_declare(f'{constNifItemPtrType} {itemParentArgName}, const QLatin1String & {itemNameArgName}', f'getItem({itemParentArgName}, {itemNameArgName}{getItemReportArg})', comment_child)
    func_declare(f'{constNifItemPtrType} {itemParentArgName}, const char * {itemNameArgName}', f'getItem({itemParentArgName}, QLatin1String({itemNameArgName}){getItemReportArg})', comment_child)
    # print(f'{indent}')
    func_declare(f'const QModelIndex & {indexArgName}', f'getItem({indexArgName})', comment_index)
    func_declare(f'const QModelIndex & {indexParentArgName}, int {itemIndexArgName}', f'getItem({indexParentArgName}, {itemIndexArgName}{getItemReportArg})', comment_child)
    func_declare(f'const QModelIndex & {indexParentArgName}, const QString & {itemNameArgName}', f'getItem({indexParentArgName}, {itemNameArgName}{getItemReportArg})', comment_child)
    func_declare(f'const QModelIndex & {indexParentArgName}, const QLatin1String & {itemNameArgName}', f'getItem({indexParentArgName}, {itemNameArgName}{getItemReportArg})', comment_child)
    func_declare(f'const QModelIndex & {indexParentArgName}, const char * {itemNameArgName}', f'getItem({indexParentArgName}, QLatin1String({itemNameArgName}){getItemReportArg})', comment_child)

    if (flags & FLAG_TEMPLATE) and funcCallName == func_name:
        funcCallName += '<T>'

    indent = ''
    print(f'{indent}')
    for base_args, get_item_code in func_bodies:
        # print(f'{indent}')
        print(f'{indent}{templatePrefix}inline {return_type} {class_name}::{func_name}( {base_args}{extraArgDeclare2} ){constMod}')
        print(f'{indent}{openBracket}')
        print(f'{indent}\t{returnOp}{funcCallName}( {get_item_code}{extraArgList} );')
        print(f'{indent}{closeBracket}')
        for xt in extra_template_types:
            print(f'{indent}template <> inline {xt.return_type} {class_name}::{func_name}( {base_args}{xt.arg_declare} ){constMod}')
            print(f'{indent}{openBracket}')
            print(f'{indent}\t{returnOp}{xt.func_call}( {get_item_code}{extraArgList} );')
            print(f'{indent}{closeBracket}')

"""
generate(
    class_name = 'BaseModel',
    func_name = 'getIndex',
    return_type = 'QModelIndex',
    comment = 'Get the model index of an item',
    flags = FLAG_CONST,
    func_call = 'itemToIndex',
    extra_args = ExtraArg('int', 'column', default_val = '0')
)
"""

"""
generate(
    class_name = 'BaseModel',
    func_name = 'updateArraySize',
    return_type = 'bool',
    comment = 'Update the size of an array from its conditions (append missing or remove excess items)',
    flags = FLAG_ARRAY_ARGS | FLAG_ARRAY_COMMENTS,
    func_call = 'updateArraySizeImpl'
)
"""

"""
generate(
    class_name = 'BaseModel',
    func_name = 'get',
    return_type = 'T',
    comment = 'Get the value of an item',
    flags = FLAG_CONST | FLAG_TEMPLATE,
    func_call = 'NifItem::get<T>'
)
"""

"""
generate(
    class_name = 'BaseModel',
    func_name = 'set',
    return_type = 'bool',
    comment = 'Set the value of an item',
    flags = FLAG_TEMPLATE | FLAG_GETITEM_REPORT,
    extra_args = ExtraArg('const T &', 'val')
)
"""

"""
generate(
    class_name = 'BaseModel',
    func_name = 'getArray',
    return_type = 'QVector<T>',
    comment = 'Get an array as a QVector',
    flags = FLAG_CONST | FLAG_TEMPLATE | FLAG_ARRAY_ARGS | FLAG_ARRAY_COMMENTS,
    func_call = 'NifItem::getArray<T>'
)
"""

"""
generate(
    class_name = 'BaseModel',
    func_name = 'setArray',
    return_type = 'void',
    comment = 'Write a QVector to an array',
    flags = FLAG_TEMPLATE | FLAG_ARRAY_ARGS | FLAG_ARRAY_COMMENTS | FLAG_GETITEM_REPORT,
    extra_args = ExtraArg('const QVector<T> &', 'array')
)
"""

"""
generate(
    class_name = 'BaseModel',
    func_name = 'fillArray',
    return_type = 'void',
    comment = 'Fill an array with a value',
    flags = FLAG_TEMPLATE | FLAG_ARRAY_ARGS | FLAG_ARRAY_COMMENTS | FLAG_GETITEM_REPORT,
    extra_args = ExtraArg('const T &', 'val')
)
"""

"""
generate(
    class_name = 'NifModel',
    func_name = 'get',
    return_type = 'T',
    comment = 'Get the value of an item',
    flags = FLAG_CONST | FLAG_TEMPLATE,
)
"""

"""
generate(
    class_name = 'NifModel',
    func_name = 'set',
    return_type = 'bool',
    comment = 'Set the value of an item',
    flags = FLAG_TEMPLATE | FLAG_GETITEM_REPORT,
    extra_args = ExtraArg('const T &', 'val'),
)
"""

"""
generate(
    class_name = 'NifModel',
    func_name = 'resolveString',
    return_type = 'QString',
    comment = 'Get the string value of an item, expanding string indices or subitems if necessary',
    flags = FLAG_CONST
)
"""

"""
generate(
    class_name = 'NifModel',
    func_name = 'assignString',
    return_type = 'bool',
    comment = 'Set the string value of an item, updating string indices or subitems if necessary',
    flags = FLAG_GETITEM_REPORT,
    extra_args = [ExtraArg('const QString &', 'string'), ExtraArg('bool', 'replace', default_val = 'false')]
)
"""

"""
generate(
    class_name = 'NifModel',
    func_name = 'getLink',
    return_type = 'qint32',
    comment = 'Return the link value (block number) of an item if it\'s a valid link, otherwise -1',
    flags = FLAG_CONST
)
"""

"""
generate(
    class_name = 'NifModel',
    func_name = 'getLinkItem',
    return_type = 'const NifItem *',
    comment = 'Return the link block item stored in an item if it\'s a valid link',
    flags = FLAG_CONST
)
"""

"""
generate(
    class_name = 'NifModel',
    func_name = 'setLink',
    return_type = 'bool',
    comment = 'Set the link value (block number) of an item if it\'s a valid link',
    flags = 0,
    extra_args = ExtraArg('qint32', 'link')
)
"""

"""
generate(
    class_name = 'NifModel',
    func_name = 'getLinkArray',
    return_type = 'QVector<qint32>',
    comment = 'Return a QVector of link values (block numbers) of an item if it\'s a valid link array',
    flags = FLAG_CONST | FLAG_ARRAY_ARGS
)
"""

"""
generate(
    class_name = 'NifModel',
    func_name = 'setLinkArray',
    return_type = 'bool',
    comment = 'Write a QVector of link values (block numbers) to an item if it\'s a valid link array.\nThe size of QVector must match the current size of the array.',
    flags = FLAG_ARRAY_ARGS,
    extra_args = ExtraArg('const QVector<qint32> &', 'links'),
)
"""