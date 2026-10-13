"""Lower actual generated behavior C to the isolated evaluation VM.

Only the Clang typed AST is accepted. Unsupported syntax fails compilation;
authored functions never silently execute their original native bodies.
"""
import argparse
import collections
import json
from pathlib import Path
import re
import sys
from lower_types import Types, MEMBER, declare, spelling, walk

sys.setrecursionlimit(20000)

GENERIC = {
    'script_signbit', 'script_state_get_float', 'script_state_set_float',
    'script_state_get_int', 'script_state_set_int', 'script_state_array_bool',
    'script_state_array_set_bool', 'script_state_array_float',
    'script_state_array_set_float', 'scripted_article_state_int',
    'scripted_article_state_float', 'scripted_article_state_bool',
    'scripted_article_set_state_int', 'scripted_article_set_state_float',
    'scripted_article_set_state_bool', 'scripted_article_creator',
    'scripted_article_attached_owner', 'scripted_article_owner_motion',
    'scripted_article_owner_state_int', 'scripted_article_owner_state_float',
    'scripted_article_owner_state_bool', 'scripted_article_set_owner_state_int',
    'scripted_article_set_owner_state_float', 'scripted_article_set_owner_state_bool',
    'scripted_article_owner_command', 'scripted_article_holder',
}
MODULE = {'FreakMelee_GetFighterModule', 'FreakMelee_SetFighterParameters',
          'FreakMelee_GetFighterModuleLayout', 'FreakMelee_GetScriptSourceHash'}
BIN = {'+':'Add','-':'Sub','*':'Mul','/':'Div','%':'Mod','&':'And','|':'Or',
       '^':'Xor','<<':'Shl','>>':'Shr','==':'Eq','!=':'Ne','<':'Lt','<=':'Le',
       '>':'Gt','>=':'Ge'}
BUILTINS={'__builtin_isfinite','__builtin_fpclassify'}

def unwrap(node):
    while node['kind'] in ('ParenExpr','ImplicitCastExpr','CStyleCastExpr'):
        node = node['inner'][0]
    return node

def endpoint(node, which):
    point = node['range'][which]
    point = point.get('expansionLoc', point)
    return point['offset'] + (point.get('tokLen', 0) if which == 'end' else 0)

class Compiler:
    def __init__(self, source, tree, layouts):
        self.source, self.tree = source, tree
        self.types = Types(tree, layouts)
        self.constants, self.constant_ids = [], {}
        self.signatures, self.signature_ids = [], {}
        self.native_calls = collections.Counter()
        self.enums = {}
        for node in walk(tree):
            if node['kind'] == 'EnumDecl':
                value = -1
                for entry in node.get('inner',[]):
                    if entry['kind'] != 'EnumConstantDecl': continue
                    exprs = list(walk(entry))
                    explicit = next((x for x in exprs if x['kind']=='ConstantExpr' and 'value' in x),None)
                    value = int(explicit['value']) if explicit else value+1
                    self.enums[entry['id']] = value
        self.local_functions = []
        self.native_generic_definitions = []
        source_names = set(re.findall(r'(?m)^(?:static\s+)?(?:inline\s+)?[A-Za-z_][\w\s*]*?\b(\w+)\s*\([^;{}]*\)\s*\{', source))
        for node in tree['inner']:
            if node['kind'] != 'FunctionDecl': continue
            if not any(x['kind']=='CompoundStmt' for x in node.get('inner',[])): continue
            name = node.get('name','')
            if name in GENERIC:
                self.native_generic_definitions.append(name);continue
            if name in MODULE: continue
            if name.startswith(('script_fn_','authored_','scripted_article_','scripted_item_','scripted_effect_')):
                self.local_functions.append(node)
            elif name in source_names:
                raise ValueError('Unclassified generated function would remain native: '+name)
        if not self.local_functions: raise ValueError('No actual authored functions found')
        self.function_ids = {node['name']:i for i,node in enumerate(self.local_functions)}
        self.functions = []

    def constant(self, text, kind):
        key = (text,kind)
        if key not in self.constant_ids:
            self.constant_ids[key] = len(self.constants)
            self.constants.append('.'+MEMBER[kind]+' = '+text)
        return self.constant_ids[key]

    def gateway(self, callee, arguments, result, label):
        ctype = spelling(callee).replace('__size_t','size_t')
        if label in BUILTINS: ctype = label
        params = [(spelling(x) if self.types.c_type(x)=='__size_t' else self.types.c_type(x), self.types.scalar(x)) for x in arguments]
        key = (ctype,tuple(params),result)
        if key not in self.signature_ids:
            self.signature_ids[key] = len(self.signatures)
            self.signatures.append((ctype,params,result))
        self.native_calls[label] += 1
        return self.signature_ids[key]

    def compile(self):
        for node in self.local_functions:
            try:
                function = Function(self,node)
                function.compile()
                self.functions.append(function)
            except Exception as error:
                raise ValueError(node['name']+': '+str(error)) from error

    def emit(self, path, native_path, manifest_path):
        wrappers, native_replacements = [], []
        for index, fn in enumerate(self.functions):
            node, body = fn.node, fn.body
            start, end = endpoint(body,'begin'), endpoint(body,'end')
            args = fn.arguments
            declarations = ', '.join('{.'+MEMBER[self.types.scalar(x)]+' = '+('(uintptr_t)' if self.types.scalar(x)=='Eval_Ptr' else '')+x['name']+'}' for x in args)
            call = 'evalVm_Run(&evaluation_context, &evaluation_program, '+str(index)+', '+('values' if args else 'NULL')+', '+str(len(args))+')'
            text = '{\n' + ('    EvalValue values[] = {'+declarations+'};\n' if args else '')
            if fn.result == 'Eval_Void': text += '    (void)'+call+';\n'
            else: text += '    return ('+fn.return_ctype+')'+call+'.'+MEMBER[fn.result]+';\n'
            text += '}'
            wrappers.append((start,end,text))
            native_replacements.append((start,start+1,'{ ++evaluation_native_entries['+str(index)+'];'))
        def replace(replacements):
            source = self.source
            for start,end,text in sorted(replacements, reverse=True): source = source[:start]+text+source[end:]
            return source
        head = '#include "vm_runtime.h"\nstatic EvalContext evaluation_context;\nstatic const EvalProgram evaluation_program;\n'
        output = head+replace(wrappers)+'\n'+self.emit_program()
        output += self.emit_stats(False)
        path.write_text(output, newline='\n')
        native_path.write_text('#include "vm_runtime.h"\nstatic uint64_t evaluation_native_entries['+str(len(self.functions))+'];\n'+replace(native_replacements)+self.emit_stats(True), newline='\n')
        manifest = {'authored_function_count':len(self.functions),
            'functions':[{'name':f.node['name'],'instructions':len(f.code),'registers':f.registers,'local_bytes':f.local_bytes,'arguments':len(f.arguments)} for f in self.functions],
            'instruction_count':sum(len(f.code) for f in self.functions),
            'constant_count':len(self.constants),'typed_native_gateways':len(self.signatures),
            'native_call_sites':dict(self.native_calls),
            'generic_native_whitelist':sorted(set(self.native_generic_definitions)), 'native_module_setup_whitelist':sorted(MODULE),
            'authored_native_fallbacks':[], 'boundary':'All selected authored bodies interpreted; generated fixed ingress preserves the existing C callback ABI. Native generic accessors/import and typed Host operations are counted separately.'}
        manifest_path.write_text(json.dumps(manifest,indent=2)+'\n',newline='\n')

    def emit_stats(self, native):
        count = len(self.functions)
        entries = 'evaluation_native_entries' if native else 'evaluation_context.function_entries'
        text = '\n__declspec(dllexport) unsigned __cdecl FreakMelee_EvalFunctionCount(void) { return '+str(count)+'; }\n'
        text += '__declspec(dllexport) const char* __cdecl FreakMelee_EvalFunctionName(unsigned n) {\n    static const char* names[] = {'+','.join(json.dumps(f.node['name']) for f in self.functions)+'}; return n < '+str(count)+' ? names[n] : NULL; }\n'
        text += '__declspec(dllexport) uint64_t __cdecl FreakMelee_EvalFunctionEntries(unsigned n) { return n < '+str(count)+' ? '+entries+'[n] : 0; }\n'
        text += '__declspec(dllexport) void __cdecl FreakMelee_EvalResetStats(void) { '
        if native: text += 'memset(evaluation_native_entries, 0, sizeof(evaluation_native_entries));'
        else: text += 'evaluation_context.entries = evaluation_context.native_calls = evaluation_context.interpreted_calls = evaluation_context.errors = 0; memset(evaluation_context.function_entries, 0, sizeof(evaluation_context.function_entries));'
        text += ' }\n'
        text += '__declspec(dllexport) void __cdecl FreakMelee_EvalStats(uint64_t out[4]) { '
        if native: text += 'out[0] = 0; for (unsigned i = 0; i < '+str(count)+'; ++i) out[0] += evaluation_native_entries[i]; out[1] = out[2] = out[3] = 0;'
        else: text += 'out[0] = evaluation_context.entries; out[1] = evaluation_context.native_calls; out[2] = evaluation_context.interpreted_calls; out[3] = evaluation_context.errors;'
        return text+' }\n'

    def emit_program(self):
        text = '\nstatic EvalValue evaluation_native_call(unsigned signature, uintptr_t callee, const EvalValue* args) {\n    EvalValue value = {0};\n    switch (signature) {\n'
        for index,(ctype,params,result) in enumerate(self.signatures):
            args = ', '.join('('+p+')args['+str(i)+'].'+MEMBER[t] for i,(p,t) in enumerate(params))
            expr = ctype+'('+args+')' if ctype in BUILTINS else '(('+ctype+')callee)('+args+')'
            text += '    case '+str(index)+': '
            if result == 'Eval_Void': text += '(void)'+expr+';'
            else: text += 'value.'+MEMBER[result]+' = '+('(uintptr_t)' if result=='Eval_Ptr' else '')+expr+';'
            text += ' return value;\n'
        text += '    default: ++evaluation_context.errors; return value;\n    }\n}\n'
        text += 'static const EvalValue evaluation_constants[] = {\n'+''.join('    {'+x+'},\n' for x in self.constants)+'};\n'
        text += 'static const EvalInstruction evaluation_code[] = {\n'
        for fn in self.functions:
            text += '    // '+fn.node['name']+'\n'
            for insn in fn.code:
                text += '    {'+', '.join(map(str,insn))+'},\n'
        text += '};\nstatic const EvalFunction evaluation_functions[] = {\n'
        first = 0
        for fn in self.functions:
            text += '    {'+', '.join(map(str,[first,len(fn.code),fn.registers,fn.local_bytes,len(fn.arguments),fn.result]))+', '+json.dumps(fn.node['name'])+'},\n'
            first += len(fn.code)
        text += '};\nstatic const EvalProgram evaluation_program = {evaluation_code, evaluation_functions, evaluation_constants, evaluation_native_call, '+str(first)+', '+str(len(self.constants))+', '+str(len(self.functions))+'};\n'
        return text

class Function:
    def __init__(self, compiler, node):
        self.compiler,self.node,self.types = compiler,node,compiler.types
        self.arguments = [x for x in node.get('inner',[]) if x['kind']=='ParmVarDecl']
        self.body = next(x for x in node['inner'] if x['kind']=='CompoundStmt')
        self.return_ctype = spelling(node).split(' (',1)[0]
        self.result = self.types.scalar({'type':{'qualType':self.return_ctype}})
        self.code,self.locals = [],{}
        self.registers = max(1,len(self.arguments))
        self.local_bytes = 0

    def reg(self, count=1):
        value = self.registers; self.registers += count
        if self.registers > 2048: raise ValueError('Register capacity exceeded '+str(self.registers))
        return value

    def emit(self, op, kind='Eval_Void', dst=0, a=0, b=0, c=0, immediate=0):
        self.code.append(['Eval_'+op,kind,dst,a,b,c,immediate])
        return len(self.code)-1

    def jump(self, op, condition=None):
        return self.emit(op, condition[1] if condition else 'Eval_Void', b=condition[0] if condition else 0)

    def patch(self, index, target=None):
        self.code[index][3] = len(self.code) if target is None else target

    def constant(self, text, kind='Eval_U64'):
        register = self.reg()
        self.emit('Constant',kind,register,a=self.compiler.constant(text,kind))
        return register,kind

    def allocate(self, node):
        size,alignment = self.types.size(node)
        offset = (self.local_bytes+alignment-1)//alignment*alignment
        self.local_bytes = offset+size
        if self.local_bytes > 8192: raise ValueError('Local byte capacity exceeded')
        register = self.reg();self.emit('LocalAddress','Eval_Ptr',register,immediate=offset)
        return register

    def add_offset(self, address, offset):
        if offset == '0': return address
        off,_ = self.constant(offset,'Eval_U64')
        register = self.reg();self.emit('Add','Eval_Ptr',register,address,off)
        return register

    def address(self, node):
        kind, children = node['kind'], node.get('inner',[])
        if kind in ('ParenExpr','ImplicitCastExpr') and node.get('castKind') != 'LValueToRValue': return self.address(children[0])
        if kind == 'DeclRefExpr':
            declaration = node['referencedDecl']; name = declaration['name']
            if declaration['id'] in self.locals: return self.locals[declaration['id']],None
            if declaration['kind']=='VarDecl': return self.constant('(uintptr_t)&'+name,'Eval_Ptr')[0],None
            if declaration['kind']=='FunctionDecl': return self.constant('(uintptr_t)&'+name,'Eval_Ptr')[0],None
            raise ValueError('Unsupported address '+str(declaration))
        if kind == 'MemberExpr':
            base = self.expr(children[0])[0] if node.get('isArrow') else self.address(children[0])[0]
            offset,bits = self.types.field(node)
            return self.add_offset(base,offset),bits
        if kind == 'ArraySubscriptExpr':
            base = self.expr(children[0])[0]
            index,index_type = self.expr(children[1]); size,_ = self.types.size(node)
            scale,_ = self.constant(str(size),'Eval_I64')
            if index_type != 'Eval_I64':
                converted=self.reg();self.emit('Convert','Eval_I64',converted,index,immediate=index_type);index=converted
            offset=self.reg();self.emit('Mul','Eval_I64',offset,index,scale)
            result=self.reg();self.emit('Add','Eval_Ptr',result,base,offset)
            return result,None
        if kind == 'UnaryOperator' and node['opcode']=='*': return self.expr(children[0])[0],None
        if kind == 'CompoundLiteralExpr':
            address = self.allocate(node);self.initialize(address,children[0]);return address,None
        if kind == 'StringLiteral':return self.expr(node)[0],None
        raise ValueError('Unsupported lvalue '+kind+' '+str(node.get('name','')))

    def load(self, node):
        address,bits = self.address(node)
        if self.types.aggregate(node): return address,'Eval_Ptr'
        kind = self.types.scalar(node);result=self.reg()
        self.emit('LoadBits' if bits is not None else 'Load',kind,result,address,immediate=bits or 0)
        return result,kind

    def store(self,node,value):
        address,bits = self.address(node)
        if self.types.aggregate(node):
            self.emit('Copy',a=address,b=value[0],immediate=self.types.size(node)[0]);return
        kind=self.types.scalar(node)
        self.emit('StoreBits' if bits is not None else 'Store',kind,a=address,b=value[0],immediate=bits or 0)

    def initialize(self,address,node):
        kind = node['kind']
        if kind in ('InitListExpr','ImplicitValueInitExpr'):
            size,_=self.types.size(node);self.emit('Clear',a=address,immediate=size)
            if kind=='ImplicitValueInitExpr': return
            text=self.types.resolve(spelling(node));children=node.get('inner',[])
            if '[' in text:
                base=text[:text.index('[')].strip();shape=re.findall(r'\[(\d+)\]',text)
                element=base+''.join('['+v+']' for v in shape[1:]);stride,_=self.types.size(element)
                for i,child in enumerate(children):self.initialize(self.add_offset(address,str(i*stride)),child)
            else:
                record=self.types.records.get(text)
                if not record:raise ValueError('Initializer record missing '+text)
                fields=[x for x in record['inner'] if x['kind']=='FieldDecl']
                for field,child in zip(fields,children):
                    self.initialize(self.add_offset(address,'offsetof('+self.types.c_type(node)+', '+field['name']+')'),child)
            return
        value=self.expr(node);self.emit('Store',self.types.scalar(node),a=address,b=value[0])

    def expr(self,node):
        kind, children=node['kind'],node.get('inner',[])
        if kind in ('ParenExpr','ConstantExpr'): return self.expr(children[0])
        if kind in ('IntegerLiteral','FloatingLiteral'):
            return self.constant(node['value'],self.types.scalar(node))
        if kind=='StringLiteral':return self.constant('(uintptr_t)'+node['value'],'Eval_Ptr')
        if kind=='ImplicitValueInitExpr':return self.constant('0',self.types.scalar(node))
        if kind in ('ImplicitCastExpr','CStyleCastExpr'):
            cast=node['castKind']
            if cast=='LValueToRValue':return self.load(children[0])
            if cast in ('ArrayToPointerDecay','FunctionToPointerDecay'):return self.address(children[0])[0],'Eval_Ptr'
            value,old=self.expr(children[0]);new=self.types.scalar(node)
            if old==new:return value,new
            result=self.reg();self.emit('Convert',new,result,value,immediate=old);return result,new
        if kind=='DeclRefExpr':
            ref=node['referencedDecl']
            if ref['kind']=='EnumConstantDecl':return self.constant(str(self.compiler.enums[ref['id']]),self.types.scalar(node))
            return self.load(node)
        if kind in ('MemberExpr','ArraySubscriptExpr'):return self.load(node)
        if kind=='CompoundLiteralExpr':return self.address(node)[0],'Eval_Ptr'
        if kind=='UnaryExprOrTypeTraitExpr':
            if node['name']!='sizeof':raise ValueError('Unsupported trait '+node['name'])
            ctype=node.get('argType',{}).get('qualType') or self.types.c_type(children[0])
            return self.constant('sizeof('+ctype+')',self.types.scalar(node))
        if kind=='UnaryOperator':
            op=node['opcode']
            if op=='&':return self.address(children[0])[0],'Eval_Ptr'
            if op=='*':return self.load(node)
            value,typ=self.expr(children[0]);result=self.reg()
            if op=='+':return value,typ
            if op in ('-','!','~'):
                self.emit({'-':'Neg','!':'Not','~':'Invert'}[op],typ,result,value)
                return result,self.types.scalar(node)
            if op in ('++','--'):
                one,_=self.constant('1',typ);self.emit('Add' if op=='++' else 'Sub',typ,result,value,one)
                self.store(children[0],(result,typ));return (value if node.get('isPostfix') else result),typ
            raise ValueError('Unsupported unary '+op)
        if kind=='BinaryOperator':
            op=node['opcode']
            if op=='=':
                value=self.expr(children[1]);self.store(children[0],value);return value
            if op==',':self.expr(children[0]);return self.expr(children[1])
            if op in ('&&','||'):
                typ=self.types.scalar(node);result,_=self.constant('0' if op=='&&' else '1',typ)
                left=self.expr(children[0]);skip=self.jump('JumpFalse' if op=='&&' else 'JumpTrue',left)
                right=self.expr(children[1]);converted=self.reg();self.emit('Convert',typ,converted,right[0],immediate=right[1])
                # Logical operators normalize rather than copy arbitrary int.
                zero,_=self.constant('0',right[1]);self.emit('Ne',right[1],result,right[0],zero)
                self.patch(skip);return result,typ
            left,lt=self.expr(children[0]);right,rt=self.expr(children[1]);typ=self.types.scalar(node)
            if op in ('+','-') and lt=='Eval_Ptr' and rt!='Eval_Ptr':
                ctype=self.types.c_type(children[0]);scale,_=self.constant('sizeof(*(('+ctype+')0))','Eval_I64')
                converted=self.reg();self.emit('Convert','Eval_I64',converted,right,immediate=rt)
                amount=self.reg();self.emit('Mul','Eval_I64',amount,converted,scale);right=amount
            result=self.reg();self.emit(BIN[op],lt,result,left,right)
            return result,typ
        if kind=='ConditionalOperator':
            condition=self.expr(children[0]);otherwise=self.jump('JumpFalse',condition);typ=self.types.scalar(node);result=self.reg()
            value=self.expr(children[1]);self.emit('Move',typ,result,value[0]);done=self.jump('Jump');self.patch(otherwise)
            value=self.expr(children[2]);self.emit('Move',typ,result,value[0]);self.patch(done);return result,typ
        if kind=='StmtExpr':
            body=children[0];value=None
            for child in body.get('inner',[]):value=self.stmt(child)
            return value or (self.constant('0','Eval_U64') if self.types.scalar(node)=='Eval_Void' else self.fail('Statement expression has no value'))
        if kind=='CallExpr':
            callee=unwrap(children[0]);args=children[1:]
            name=callee.get('referencedDecl',{}).get('name') if callee['kind']=='DeclRefExpr' else None
            values=[self.expr(x) for x in args]
            start=self.reg(len(values)) if values else 0
            for i,value in enumerate(values):self.emit('Move',value[1],start+i,value[0])
            result=self.reg();typ=self.types.scalar(node) if not self.types.aggregate(node) else 'Eval_Void'
            if name in self.compiler.function_ids:
                self.emit('Call',typ,result,self.compiler.function_ids[name],start,len(values))
            else:
                if name and name.startswith(('script_fn_','authored_','scripted_article_','scripted_item_')) and name not in GENERIC:
                    raise ValueError('Authored native fallback forbidden '+name)
                label=name or callee.get('name') or ('Host array '+spelling(callee))
                ptr=self.constant('(uintptr_t)&evaluation_native_call','Eval_Ptr')[0] if label in BUILTINS else self.expr(children[0])[0]
                sig=self.compiler.gateway(children[0],args,typ,label)
                self.emit('CallNative',typ,result,ptr,start,len(values),sig)
            return result,typ
        raise ValueError('Unsupported expression '+kind+' '+str(node.get('opcode','')))

    @staticmethod
    def fail(message):raise ValueError(message)

    def stmt(self,node):
        kind,children=node['kind'],node.get('inner',[])
        if kind=='CompoundStmt':
            for child in children:self.stmt(child)
            return None
        if kind=='DeclStmt':
            for decl in children:
                if decl['kind']!='VarDecl':raise ValueError('Unsupported declaration '+decl['kind'])
                address=self.allocate(decl);self.locals[decl['id']]=address
                if decl.get('inner'):self.initialize(address,decl['inner'][-1])
            return None
        if kind=='IfStmt':
            if node.get('hasInit') or node.get('hasVar'):raise ValueError('Unexpected conditional declaration')
            condition=self.expr(children[0]);otherwise=self.jump('JumpFalse',condition);self.stmt(children[1])
            if len(children)==3:
                done=self.jump('Jump');self.patch(otherwise);self.stmt(children[2]);self.patch(done)
            else:self.patch(otherwise)
            return None
        if kind=='WhileStmt':
            top=len(self.code);condition=self.expr(children[0]);done=self.jump('JumpFalse',condition);self.stmt(children[1]);self.emit('Jump',a=top);self.patch(done);return None
        if kind=='ReturnStmt':
            value=self.expr(children[0]) if children else (0,'Eval_Void');self.emit('Return',value[1],a=value[0]);return None
        if kind=='NullStmt':return None
        return self.expr(node)

    def compile(self):
        for index,argument in enumerate(self.arguments):
            address=self.allocate(argument);self.locals[argument['id']]=address
            self.emit('Store',self.types.scalar(argument),a=address,b=index)
        self.stmt(self.body)
        self.emit('Return','Eval_Void' if self.result=='Eval_Void' else self.result,a=0)
        # Any patched fall-through target reaches the final neutral return.

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source',type=Path,required=True)
    parser.add_argument('--ast',type=Path,required=True)
    parser.add_argument('--layouts',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    output=args.output.resolve()
    if output.drive.upper()!='A:':parser.error('Evaluation output must be on A:')
    output.mkdir(parents=True,exist_ok=True)
    compiler=Compiler(args.source.read_text(),json.loads(args.ast.read_text(encoding='utf-8-sig')),args.layouts.read_text())
    compiler.compile()
    compiler.emit(output/'interpreted.c',output/'native.c',output/'lowering.json')
    print(json.dumps({'functions':len(compiler.functions),'instructions':sum(len(f.code) for f in compiler.functions),'gateways':len(compiler.signatures)}))

if __name__=='__main__':main()
