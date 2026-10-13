"""Read exact retail Squall command graphs; no source/runtime mutation."""
import argparse,hashlib,json,re,struct,sys
from pathlib import Path

def main():
 p=argparse.ArgumentParser()
 for n in ('root','iso','output'):p.add_argument('--'+n,type=Path,required=True)
 x=p.parse_args();root=x.root.resolve();sys.path.insert(0,str(root/'tools'))
 from extract_native_animation import extract_clip
 common=(root/'src/melee/ft/kinds/ftCommon/forward.h').read_text(encoding='utf-8')
 section=common[common.index('typedef enum ftCo_Submotion'):];section=section[:section.index('} ftCo_Submotion;')]
 value=-1
 for field in section[section.index('{')+1:].split(','):
  field=field.strip()
  if not field:continue
  if '=' in field:name,raw=field.split('=');value=int(raw.strip(),0)
  else:name=field;value+=1
  if name.strip()=='ftCo_SM_Count':count=value
 widths_text=(root/'src/melee/ft/ftaction_data.c').read_text(encoding='utf-8')
 widths=list(map(int,re.search(r'action_widths\[49\] = \{(.*?)\};',widths_text,re.S)[1].replace('\n','').strip().strip(',').split(',')))
 result={'scope':'Retail Squall command graph census, no scheduling approximation','common_submotion_count':count,'forms':{}}
 for suffix,name in [('Pp','Popo'),('Nn','Nana')]:
  data,symbol=extract_clip(x.iso.resolve(),'Pl'+suffix+'.dat','ftData'+name)
  def word(at):return struct.unpack_from('>I',data,32+at)[0]
  table=word(symbol+12);records=[]
  for extra in range(2,6):
   submotion=count+extra;entry=table+submotion*24;start=word(entry+12)
   pending=[start] if start else [];seen=set();commands=[]
   while pending:
    at=pending.pop()
    while at and at not in seen:
     if at>=table or at%4:raise ValueError('Malformed command target')
     seen.add(at);head=word(at);op=head>>26
     width=2 if op in (5,7) else 1 if op<10 else widths[op-10] if op<=58 else 0
     if not width:raise ValueError('Unknown opcode')
     raw=[word(at+i*4) for i in range(width)]
     row={'offset':at,'opcode':op,'words':raw}
     if op==10:row.update(gfx_id=raw[1]>>16,raw_part=(head>>18)&255,destroy_on_state_change=bool((head>>15)&1))
     commands.append(row)
     if op in (5,7):pending.append(raw[1])
     if op in (0,6,7):break
     at+=width*4
   records.append({'motion':341+extra,'submotion':submotion,'command_start':start,'clip_presence':word(entry+20),'commands':commands})
  result['forms'][name]={'data_sha256':hashlib.sha256(data).hexdigest(),'animation_table':table,'motions':records}
 sources=['src/melee/ft/ftaction_data.c','src/melee/ft/kinds/ftCommon/forward.h','src/melee/ft/kinds/ftCommon/ftCo_09F7.c','src/melee/ft/kinds/ftPopo/ftpopospecials.c','src/melee/ef/efsync.c']
 result['sources']={s:hashlib.sha256((root/s).read_bytes()).hexdigest() for s in sources}
 x.output.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
 print([(name,[(r['motion'],[c['gfx_id'] for c in r['commands'] if c['opcode']==10]) for r in f['motions']]) for name,f in result['forms'].items()])
if __name__=='__main__':main()
