"""Read-only demo archive provenance; deliberately no importer or gameplay claim."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def clips(data,root):
    end=32+struct.unpack_from('>I',data,4)[0]
    cursor=32+root
    records=[]
    while cursor<end:
        if data[cursor]==255:
            cursor+=1
            continue
        size,length,relocs,roots,externs=struct.unpack_from('>5I',data,cursor)
        assert size>=32 and cursor+size<=end
        blob=data[cursor:cursor+size]
        table=32+length+4*relocs
        strings=table+8*(roots+externs)
        for i in range(roots):
            offset,name=struct.unpack_from('>II',blob,table+8*i)
            at=strings+name
            symbol=blob[at:blob.index(0,at)].decode('ascii')
            records.append({'symbol':symbol,'archive_offset':cursor,'root':offset,
                            'bytes':size,'sha256':hashlib.sha256(blob).hexdigest()})
        cursor+=size
    return records


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--iso',type=Path,required=True)
    p.add_argument('--pipeline-receipt',type=Path,required=True)
    p.add_argument('--source-root',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    a=p.parse_args();out=a.output.resolve();source=a.source_root.resolve()
    if out.drive.upper()!='A:' or out.exists():p.error('Fresh A: output required')
    sys.path.insert(0,str(ROOT/'tools'))
    from extract_native_animation import extract_clip
    receipt=json.loads(a.pipeline_receipt.read_text(encoding='utf-8'))
    report={}
    for member,short,name in [('zelda','Zd','Zelda'),('sheik','Sk','Seak')]:
        archives=[]
        for archive,symbol in [('GmRstM'+short+'.dat','ftDemoResultMotionFile'+name),
                               ('Pl'+short+'DViWaitAJ.dat','ftDemoViWaitMotionFile'+name),
                               ('IrAls.dat','ftDemoIntroMotionFile'+name),
                               ('GmRegEnd.dat','ftDemoEndingMotionFile'+name)]:
            data,root=extract_clip(a.iso,archive,symbol)
            archives.append({'archive':archive,'bank_symbol':symbol,'bank_root':root,
                'archive_bytes':len(data),'archive_sha256':hashlib.sha256(data).hexdigest(),
                'embedded_clips':clips(data,root)})
        entries=receipt['members'][member]['animation_inventory']['missing_demo_clips']
        for entry in entries:
            entry['disc_locations']=[{'archive':bank['archive'],**clip} for bank in archives
                for clip in bank['embedded_clips'] if clip['symbol']==entry['symbol']]
        assert all(x['disc_locations'] for x in entries),entries
        report[member]={'absent_packaged_demo_entries':entries,'disc_banks':archives}
    relative=['src/melee/ft/native_fighter_runtime.c','src/melee/ft/ftcharacter_assets.c',
              'src/melee/ft/ftdemo.c','src/melee/ft/ftdata.c','src/melee/ft/ftanimation_bank.c',
              'src/melee/pl/player.c','src/melee/gm/gm_1601.c','src/melee/gm/gm_1798.c',
              'src/melee/gm/gm_1832.c','src/melee/vi/vi.c']
    result={'status':'READ-ONLY exact disc provenance, no demo runtime acceptance','members':report,
            'source_root':str(source),'source_sha256':{r:sha(source/r) for r in relative},
            'pipeline_receipt_sha256':sha(a.pipeline_receipt)}
    out.mkdir(parents=True)
    (out/'receipt.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps({k:len(v['absent_packaged_demo_entries']) for k,v in report.items()}))


if __name__=='__main__':main()
