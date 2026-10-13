"""Read-only serialized output audit. No archive rewriting/materialization."""
import hashlib
import struct


def sha(data):
    return hashlib.sha256(data).hexdigest()


class Dat:
    def __init__(self, raw):
        size, data, relocs, roots, externs = struct.unpack_from('>5I', raw)
        assert size == len(raw)
        self.raw=raw; self.data=raw[32:32+data]
        at=32+data; self.relocs=set(struct.unpack_from('>'+str(relocs)+'I',raw,at));at+=relocs*4
        assert len(self.relocs)==relocs
        self.root_bytes=raw[at:at+roots*8];strings=at+8*(roots+externs)
        self.roots={};self.external={}
        for i in range(roots+externs):
            offset,name=struct.unpack_from('>II',raw,at+i*8)
            text=raw[strings+name:raw.index(0,strings+name)].decode('ascii')
            if i<roots:assert text not in self.roots;self.roots[text]=offset
            else:
                while offset != 0xffffffff:
                    assert offset not in self.external
                    self.external[offset]=text;offset=self.word(offset)
    def word(self, at):return struct.unpack_from('>I',self.data,at)[0]
    def reference(self, at):
        if at in self.relocs:return ('local',self.word(at))
        if at in self.external:return ('external',self.external[at])
        assert self.word(at)==0,(at,self.word(at));return ('null',0)
    def text(self, at):return self.data[at:self.data.index(0,at)].decode('ascii')


def bank(raw):
    result={};at=0
    while at<len(raw):
        if raw[at]==255:at+=1;continue
        length=struct.unpack_from('>I',raw,at)[0];assert length>=32 and at+length<=len(raw)
        blob=raw[at:at+length];dat=Dat(blob)
        for name,root in dat.roots.items():
            assert name not in result;result[name]=(at,root,blob)
        at+=length
    return result


def audit(retail, popo, nana):
    p=Dat(retail['PlPp.dat']);n=Dat(retail['PlNn.dat']);o=Dat(nana['fighter.dat'])
    assert popo['fighter.dat']==retail['PlPp.dat'] and popo['animation.dat']==retail['PlPpAJ.dat']
    assert popo['costume.dat']==retail['PlPpNr.dat'] and nana['costume.dat']==retail['PlNnNr.dat']
    nr=n.roots['ftDataNana'];pr=p.roots['ftDataPopo'];assert o.roots==n.roots and 'ftDataPopo' not in o.roots
    nt=n.word(nr+12);pt=p.word(pr+12);assert o.word(nr+12)==nt
    source_base=o.word(nr+72)-p.word(pr+72);assert source_base>=len(n.data) and source_base%32==0
    # One original root pointer plus three fields of filled descriptors may change.
    allowed=set(range(nr+72,nr+76));rows=[];used=set();filled=own=holes=0
    original_bank=bank(retail['PlNnAJ.dat']);source_bank=bank(retail['PlPpAJ.dat']);produced=bank(nana['animation.dat'])
    assert nana['animation.dat'].startswith(retail['PlNnAJ.dat'])
    for i in range(321):
        at=nt+i*24;src=pt+i*24;original_size=n.word(at+8);source_size=p.word(src+8)
        if original_size:origin=n;entry=at;expected=original_bank;own+=1
        elif source_size:origin=p;entry=src;expected=source_bank;filled+=1;allowed.update(range(at,at+12))
        else:
            assert o.data[at:at+24]==n.data[at:at+24];holes+=1;continue
        symbol=origin.text(origin.word(entry));offset,root,blob=produced[symbol]
        assert symbol==o.text(o.word(at)) and o.word(at+4)==offset and o.word(at+8)==len(blob)
        assert blob==expected[symbol][2] and root==expected[symbol][1]
        assert o.data[at+12:at+24]==n.data[at+12:at+24]
        used.add(symbol);rows.append(dict(index=i,symbol=symbol,tag=o.word(at+16)&63,bytes=len(blob),sha256=sha(blob)))
    assert (own,filled,holes)==(8,260,53) and len(produced)==207 and len(used-original_bank.keys())==199
    assert set(produced)==set(original_bank)|used
    for i,value in enumerate(n.data):
        if i not in allowed:assert o.data[i]==value,('Nana original byte',i)
    # Every copied source pointer retains its exact meaning. Every other source
    # byte is literal-identical, including all article scalar/command payloads.
    pointers=set()
    for slot in p.relocs:
        assert o.reference(source_base+slot)==('local',source_base+p.word(slot));pointers.update(range(slot,slot+4))
    for slot,name in p.external.items():
        assert o.reference(source_base+slot)==('external',name);pointers.update(range(slot,slot+4))
    for i,value in enumerate(p.data):
        if i not in pointers:assert o.data[source_base+i]==value,('copied source byte',i)
    for slot in n.relocs:
        if slot!=nr+72:assert o.reference(slot)==n.reference(slot)
    for slot,name in n.external.items():assert o.reference(slot)==('external',name)
    demo=n.word(nr+20);assert o.data[demo:demo+14*24]==n.data[demo:demo+14*24]
    blend=n.word(nr+16);assert o.data[blend:blend+321*2]==n.data[blend:blend+321*2]
    article=p.word(pr+72)
    article_rows=[{'slot':i,'retail_descriptor':p.word(article+i*4),'owned_descriptor':o.word(source_base+article+i*4)} for i in range(3)]
    for row in article_rows:assert row['owned_descriptor']==source_base+row['retail_descriptor']
    return {'filled_actions':filled,'original_actions':own,'empty_actions':holes,'bank_symbols':len(produced),
        'copied_source_blobs':199,'source_data_base':source_base,'source_data_bytes':len(p.data),
        'retained_foreign_tags':[r for r in rows if r['tag']==33],'articles':article_rows,'clips':rows,
        'original_nana_nonpayload_bytes_preserved':len(n.data)-len(allowed),
        'popo_raw_files_unchanged':['fighter.dat','animation.dat','costume.dat'],
        'original_nana_demo_and_blend_bytes_preserved':True}
