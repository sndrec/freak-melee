"""Clang type/layout facts for the isolated behavior evaluator."""
import re

TYPES = {
    'void':'Eval_Void', '_Bool':'Eval_Bool', 'bool':'Eval_Bool',
    'char':'Eval_I8', 'signed char':'Eval_I8', 'unsigned char':'Eval_U8',
    'short':'Eval_I16', 'unsigned short':'Eval_U16',
    'int':'Eval_I32', 'unsigned int':'Eval_U32', 'long':'Eval_I32',
    'unsigned long':'Eval_U32', 'long long':'Eval_I64',
    'unsigned long long':'Eval_U64', 'float':'Eval_F32', 'double':'Eval_F64',
}
MEMBER = {'Eval_Bool':'u','Eval_I8':'i','Eval_U8':'u','Eval_I16':'i','Eval_U16':'u',
          'Eval_I32':'i','Eval_U32':'u','Eval_I64':'i','Eval_U64':'u',
          'Eval_F32':'f','Eval_F64':'d','Eval_Ptr':'p','Eval_Void':'u'}

def walk(node):
    if 'kind' in node: yield node
    for child in node.get('inner', []):
        yield from walk(child)

def spelling(node):
    value = node.get('type', {})
    return value.get('desugaredQualType', value.get('qualType', 'void'))

class Types:
    def __init__(self, tree, layouts):
        self.nodes = {n['id']:n for n in walk(tree) if 'id' in n}
        self.aliases = {n['name']:spelling(n) for n in walk(tree)
                        if n['kind'] == 'TypedefDecl' and 'name' in n}
        self.fields = {}
        self.sizes = {}
        self.record_names = {}
        self.records = {}
        file_name = ''
        record_files = {}
        for n in walk(tree):
            file_name = n.get('loc', {}).get('file', file_name).replace('\\', '/')
            if n.get('kind') == 'RecordDecl': record_files[n['id']] = file_name
        records = {}
        for match in re.finditer(r'Type: (.*?)\n\s*Layout: <ASTRecordLayout\s*'
                r'Size:(\d+)\s*Alignment:(\d+)\s*FieldOffsets: \[(.*?)\]>', layouts, re.S):
            records[match[1]] = [int(x) for x in match[4].split(',') if x.strip()]
            self.sizes[match[1]] = (int(match[2]) // 8, int(match[3]) // 8)
        for node in walk(tree):
            if node['kind'] != 'RecordDecl' or not node.get('completeDefinition'): continue
            name = node.get('name')
            key = node.get('tagUsed', 'struct') + ' ' + name if name else None
            if not name:
                loc = node.get('loc', {})
                suffix = ':'+str(loc.get('line'))+':'+str(loc.get('col'))+')'
                candidates = [k for k in records if k.endswith(suffix)]
                preferred = [k for k in candidates if record_files.get(node['id'],'') in k.replace('\\','/')]
                if len(preferred)==1: key=preferred[0]
                elif len(candidates)==1: key=candidates[0]
            fields = [x for x in node.get('inner', []) if x['kind'] == 'FieldDecl']
            offsets = records.get(key)
            for index, field in enumerate(fields):
                self.fields[field['id']] = (node, offsets[index] if offsets else None)
            if key: self.record_names[node['id']] = key
            if name: self.records[node.get('tagUsed','struct')+' '+name]=node
        # Clang names anonymous typedef-owned tags "struct Vec" in types, but
        # prints their actual source location in record layout diagnostics.
        for n in walk(tree):
            if n['kind'] == 'RecordType' and n.get('decl',{}).get('id') in self.record_names:
                key=self.record_names[n['decl']['id']]
                if key in self.sizes: self.sizes[spelling(n)] = self.sizes[key]
                self.records[spelling(n)] = self.nodes[n['decl']['id']]

    def resolve(self, text):
        text = re.sub(r'\b(const|volatile|restrict)\b', '', text).strip()
        seen = set()
        while text in self.aliases and text not in seen:
            seen.add(text); text = self.aliases[text]
            text = re.sub(r'\b(const|volatile|restrict)\b', '', text).strip()
        return re.sub(r'\s+', ' ', text)

    def scalar(self, node):
        text = self.resolve(spelling(node))
        if '*' in text or '[' in text or '(' in text: return 'Eval_Ptr'
        if text.startswith('enum '): return 'Eval_I32'
        if text in TYPES: return TYPES[text]
        raise ValueError('Unsupported scalar type ' + text)

    def aggregate(self, node):
        text = self.resolve(spelling(node))
        return '[' in text and '*' not in text or text.startswith(('struct ', 'union ')) and '*' not in text

    def size(self, node):
        text = self.resolve(spelling(node)) if isinstance(node, dict) else self.resolve(node)
        if '*' in text: return 8, 8
        if '[' in text:
            base = text[:text.index('[')].strip()
            count = 1
            for value in re.findall(r'\[(\d+)\]', text): count *= int(value)
            size, alignment = self.size(base)
            return size * count, alignment
        if text in self.sizes: return self.sizes[text]
        if text.startswith('enum '): return 4, 4
        kind = TYPES.get(text)
        if kind:
            size = 1 if kind in ('Eval_Bool','Eval_I8','Eval_U8') else 2 if kind in ('Eval_I16','Eval_U16') else 8 if kind in ('Eval_I64','Eval_U64','Eval_F64') else 4
            return size, size
        raise ValueError('Unsupported size '+text)

    def c_type(self, node):
        # Anonymous fields receive a type through their base expression rather
        # than an invented or guessed ABI spelling.
        text = node.get('type', {}).get('qualType', 'void')
        if '(unnamed' in text or '(anonymous' in text:
            if node['kind']=='DeclRefExpr':
                return '__typeof__('+node['referencedDecl']['name']+')'
            if node['kind']=='ImplicitCastExpr' and node.get('castKind')=='ArrayToPointerDecay':
                return '__typeof__(&((('+self.c_type(node['inner'][0])+') {0})[0]))'
            if node['kind'] in ('ImplicitCastExpr','ParenExpr'):
                return self.c_type(node['inner'][0])
            if node['kind'] == 'MemberExpr':
                base = node['inner'][0]
                operator = '->' if node.get('isArrow') else '.'
                return '__typeof__(((' + self.c_type(base) + ') {0})' + operator + node['name'] + ')'
            if node['kind']=='ArraySubscriptExpr':
                return '__typeof__((('+self.c_type(node['inner'][0])+') {0})[0])'
            raise ValueError('Cannot spell anonymous type ' + text)
        return text

    def field(self, node):
        record, offset = self.fields[node['referencedMemberDecl']]
        field = self.nodes[node['referencedMemberDecl']]
        if field.get('isBitfield'):
            if offset is None: raise ValueError('Missing bitfield layout for '+record.get('name','anonymous'))
            width_node = next(x for x in walk(field) if x['kind'] == 'ConstantExpr')
            width = int(width_node['value'])
            return str(offset // 8), ((width << 32) | (offset % 8))
        if offset is not None: return str(offset // 8), None
        if not node.get('name'): raise ValueError('Missing anonymous field layout '+str(record.get('loc')))
        base = self.c_type(node['inner'][0])
        if node.get('isArrow'):
            base = '__typeof__(*((' + base + ')0))'
        return 'offsetof(' + base + ', ' + node['name'] + ')', None

def declare(text, name):
    if '(*)' in text: return text.replace('(*)', '(*'+name+')', 1)
    if '(*' in text:
        return re.sub(r'\(\*\s*', '(*'+name, text, count=1)
    match = re.search(r'\[', text)
    return text[:match.start()]+' '+name+text[match.start():] if match else text+' '+name
