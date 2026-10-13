"""Exact retail paired metadata, stock callback census and real clone-route gates."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import struct
import subprocess
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def helpers():
    path = HERE.parent/'sheik_clone_activation/audit.py'
    spec = importlib.util.spec_from_file_location('retail_census',path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def base_source(member, metadata):
    short, folder, stem, kind = ('Sk','ftSeak','seak','Seak') if member=='sheik' else ('Zd','ftZelda','zelda','Zelda')
    title = member.capitalize()
    out = [f'#include "ft{member}_clone.h"', '#ifdef MELEE_NATIVE',
        f'#include <melee/ft/kinds/{folder}/ft{stem}.h>',
        *[f'#include <melee/ft/kinds/{folder}/ft{stem}special{s}.h>' for s in ('n','s','hi','lw')],
        '#include <melee/ft/types.h>', '',
        '// Proposed raw catalog base, not registered. Raw fallback remains stock.',
        '// Independent generated modules must replace gameplay callbacks.',
        f'static const NativeFighterBone ft{title}Clone_Bones[] = {{']
    out += [f"    {{ {p['role']}, {p['joint']} }}," for p in metadata['bones']]
    out += ['};','',f'static const NativeFighterArticleAsset ft{title}Clone_Articles[] = {{']
    out += [f'    {{ {name}, {slot}, -1 }},' for name,slot in metadata['articles'].items()]
    out += ['};','']
    if member=='sheik':
        out += ['static const NativeFighterPoseGraph ftSheikClone_Poses[] = {',
            '    { UINT32_C(305010971), 4, "0" },','    { UINT32_C(1363947600), 5, "0" },','};','']
    out += [f'static const Fighter_DemoStrings ft{title}Clone_DemoMotionNames = {{']
    out += [f'    "ftDemo{mode}MotionFile{kind}",' for mode in ('Result','Intro','Ending','ViWait')]
    out += ['};','',f'const NativeFighterDefinition ft{title}Clone_Definition = {{',
        f'    .code_key = "{member}_clone", .source_kind = Ft_Kind_{kind},',
        f'    .animation_source_kind = Ft_Kind_{kind},',
        '    .sound_banks = UINT64_C(1) << 27, .sound_mute_channels = 0x44, .effect_bank = 17,',
        '    .asset_layout = { .article_mask = 0x00F, .joint_graph_mask = 0x030,'
        ' .item_format = FtCharacterItems_Chain },' if member=='sheik' else
        '    .asset_layout = { .article_mask = 0x003 },',
        f'    .article_assets = ft{title}Clone_Articles, .article_asset_count = {len(metadata["articles"])},',
        f'    .bones = ft{title}Clone_Bones, .bone_count = {len(metadata["bones"])},',
        *(['    .pose_graphs = ftSheikClone_Poses, .pose_graph_count = 2,'] if member=='sheik' else []),
        f'    .demo_motion_names = &ft{title}Clone_DemoMotionNames,',
        f'    .fighter_root = "ftData{kind}", .costume_joint = "Ply{kind}5K_Share_joint",',
        f'    .costume_matanim = "Ply{kind}5K_Share_matanim_joint",',
        f'    .action_count = {metadata["actions"]}, .demo_count = 14, .joint_count = {metadata["parts"]},',
        f'    .motion_state_first = ftCo_MS_Count, .motion_state_count = ft{short}_MS_SelfCount,',
        f'    .motion_states = ft{short}_Init_MotionStateTable,',
        f'    .special_ground_state = {{ ft{short}_MS_SpecialN'+('Start' if member=='sheik' else '')+
        f', ft{short}_MS_SpecialSStart, ft{short}_MS_SpecialHiStart_0, ft{short}_MS_SpecialLw }},',
        f'    .special_air_state = {{ ft{short}_MS_SpecialAirN'+('Start' if member=='sheik' else '')+
        f', ft{short}_MS_SpecialAirSStart, ft{short}_MS_SpecialAirHiStart_0, ft{short}_MS_SpecialAirLw }},']
    out += [f'    .{field} = ft{short}_{symbol}_Enter,' for field,symbol in (
        ('special_n','SpecialN'),('special_air_n','SpecialAirN'),('special_s','SpecialS'),
        ('special_air_s','SpecialAirS'),('special_hi','SpecialHi'),('special_air_hi','SpecialAirHi'),
        ('special_lw','SpecialLw'),('special_air_lw','SpecialAirLw'))]
    return '\n'.join(out+['};','#endif',''])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('snapshot','iso','output'):
        parser.add_argument('--'+name,type=Path,required=True)
    args = parser.parse_args()
    snapshot, output = args.snapshot.resolve(),args.output.resolve()
    if output.drive.upper()!='A:' or output.exists():
        parser.error('Use a fresh output directory on A:')
    frozen=json.loads((snapshot/'freeze.json').read_text())
    def verify():
        for name,expected in frozen.items():
            if sha(snapshot/name)!=expected: raise RuntimeError('Changed frozen '+name)
    verify();output.mkdir(parents=True)
    sdk=snapshot/'sdk';helper=helpers()
    sys.path.insert(0,str(ROOT/'tools'))
    from extract_native_animation import extract_clip
    from compose import compose
    word=lambda data,offset:struct.unpack_from('>I',data,32+offset)[0]
    co,top=extract_clip(args.iso,'PlCo.dat','ftLoadCommonData')
    part_table,omission_table=word(co,top+16),word(co,top+20)
    forward=(sdk/'src/melee/ft/forward.h').read_text()
    role_source=forward[forward.index('typedef enum Fighter_Part'):forward.index('} Fighter_Part;')]
    roles=re.findall(r'(FtPart_\w+)\s*(?:,|=)',role_source)[:53]+['53']
    table_path=sdk/'src/melee/ft/ftdata.c'
    tables=helper.arrays(table_path.read_text(),r'\b(ftData_\w+|ftKindCalcIndiviParamTable)\s*\[[^\]]*\]\s*=\s*\{')
    sound=helper.arrays((sdk/'src/melee/lb/lbaudio_ax.static.h').read_text(),r'\b(lbl_803BB3C0)\s*\[[^\]]*\]\s*=\s*\{')['lbl_803BB3C0']
    members={};commands=[]
    for member,kind,character,folder,stem,short,root in (
        ('zelda',19,18,'ftZelda','zelda','Zd','Zelda'),
        ('sheik',7,19,'ftSeak','seak','Sk','Seak')):
        descriptor=word(co,part_table+kind*4);mapping=word(co,descriptor+4);parts=word(co,descriptor+8)
        bones=[{'part':i,'role':roles[i],'joint':joint} for i,joint in enumerate(co[32+mapping:32+mapping+54]) if joint!=255]
        omission=word(co,omission_table+kind*4);assert omission==0
        source_path=sdk/f'src/melee/ft/kinds/{folder}/ft{stem}.c'
        motions=helper.arrays(source_path.read_text(),r'\b(ft'+short+r'_Init_MotionStateTable)\s*\[[^\]]*\]\s*=\s*\{')[f'ft{short}_Init_MotionStateTable']
        callbacks={name:rows[kind] for name,rows in tables.items() if len(rows)==33}
        assert sound[character]=='{ 0x1B, 0x008000000 }'
        actions=int(helper.split(callbacks['ftData_Table_Unk0'][1:-1])[1])
        data,data_top=extract_clip(args.iso,'Pl'+short+'.dat','ftData'+root)
        item_table=word(data,data_top+72)
        articles=({'It_Kind_Zelda_DinFire':0,'It_Kind_Zelda_DinFire_Explode':1} if member=='zelda' else
            {'It_Kind_Seak_NeedleThrow':0,'It_Kind_Seak_NeedleHeld':1,'It_Kind_Seak_Vanish':2,'It_Kind_Seak_Chain':3})
        assert all(word(data,item_table+slot*4) for slot in articles.values())
        costumes=[]
        for suffix in ('Nr','Re','Bu','Gr','Wh'):
            archive='Pl'+short+suffix+'.dat';symbol='Ply'+root+'5K'+('' if suffix=='Nr' else suffix)
            costume,joint_root=extract_clip(args.iso,archive,symbol+'_Share_joint')
            material,material_root=extract_clip(args.iso,archive,symbol+'_Share_matanim_joint')
            assert material==costume
            visited=set()
            def visit(offset):
                if not offset:return
                assert offset not in visited;visited.add(offset)
                visit(word(costume,offset+8));visit(word(costume,offset+12))
            visit(joint_root);assert len(visited)==parts
            costumes.append({'archive':archive,'joint_symbol':symbol+'_Share_joint',
                'material_symbol':symbol+'_Share_matanim_joint','parts':len(visited),'material_root':material_root,
                'sha256':hashlib.sha256(costume).hexdigest()})
        proposed=compose(member);out=output/member;out.mkdir()
        (out/'candidate.fms').write_text(proposed,newline='\n')
        handlers=set(re.findall(r'^    on (\w+):',proposed,re.M))
        rows=[{'id':341+i,'fields':helper.split(row[1:-1]),
               'authored_phase_declarations':{phase:f'state_{341+i}_{phase}' in handlers for phase in ('animation','interrupt','physics','collision')}}
              for i,row in enumerate(motions)]
        metadata={'kind':kind,'character':character,'parts':parts,'bones':bones,'omissions':[],
            'actions':actions,'demos':14,'articles':articles,'costumes':costumes,'callbacks':callbacks,
            'motion_rows':rows,'sound_bank':27,'effect_bank':17,'source_sha256':sha(source_path),
            'dat_sha256':hashlib.sha256(data).hexdigest(),'candidate_sha256':sha(out/'candidate.fms')}
        (out/f'ft{member}_clone.c').write_text(base_source(member,metadata),newline='\n')
        (out/f'ft{member}_clone.h').write_text(f'#ifndef NATIVE_FT_{member.upper()}_CLONE_H\n#define NATIVE_FT_{member.upper()}_CLONE_H\n#include <melee/ft/native_fighter_runtime.h>\nextern const NativeFighterDefinition ft{member.capitalize()}Clone_Definition;\n#endif\n',newline='\n')
        members[member]=metadata
        command=[snapshot/'bin/native_content_tool.exe','clone-stock',args.iso,member.capitalize(),out/'normal-project','Owned pair probe']
        result=subprocess.run(list(map(str,command)),cwd=ROOT,text=True,capture_output=True,timeout=120)
        (out/'normal-clone.log').write_text(result.stdout+result.stderr,newline='\n')
        assert result.returncode==1 and 'Unsupported stock clone base' in result.stderr
        assert not (out/'normal-project').exists()
        commands.append({'command':list(map(str,command)),'exit':result.returncode,'expected_rejection':True})
    verify()
    sources=[sdk/p for p in ('src/melee/ft/ftdata.c','src/melee/ft/native_fighter_runtime.c',
        'src/melee/ft/native_fighter_project.c','src/melee/ft/native_fighter_article_bindings.c',
        'src/melee/ft/ftcharacter_assets.c','src/melee/pl/player.c','src/melee/ft/kinds/ftCommon/ftCo_Throw.c')]
    receipt={'status':'PASS retail owned metadata and real normal-clone rejection; normal paired assets pending',
        'snapshot_sha256':sha(snapshot/'freeze.json'),'members':members,'commands':commands,
        'PlCo_sha256':hashlib.sha256(co).hexdigest(),
        'source_inputs':{str(p):sha(p) for p in sources},
        'namespace_inputs':{str(p):sha(p) for p in HERE.iterdir() if p.is_file()},
        'limits':['Exact retail graph/metadata census, not actual runtime costume/article registration.',
            'Proposed independent sources do not constitute module-entry or whole lifecycle acceptance.',
            'Both real clone commands reject because catalogs are missing. No fabricated project or installed package.']}
    (output/'census.json').write_text(json.dumps(receipt,indent=2)+'\n',newline='\n')
    print(json.dumps({member:{k:metadata[k] for k in ('parts','actions','demos')} for member,metadata in members.items()}))


if __name__=='__main__':main()
