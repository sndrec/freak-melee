"""Extract ownership and command bodies used by the actual receiver fixture."""
import argparse,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'script_kinetics'))
from extract_primitives import body

def extract(root,out):
 out.mkdir(parents=True,exist_ok=True)
 owner=(root/'src/melee/it/native_item_owner.c').read_text()
 article=(root/'src/melee/it/native_article.c').read_text()
 (out/'script_receiver_identity_actual.h').write_text('\n\n'.join([body(owner,n) for n in ('nativeItem_CaptureCreator','nativeItem_Creator')]+[body(article,'nativeArticle_Creator')])+'\n')
 setup=(root/'src/melee/ft/native_script_binding_setup.c').read_text()
 pieces='\n\n'.join(body(setup,n) for n in ('error','command_index','nativeScriptBindingSetup_Read','nativeScriptBindingSetup_Write'))
 (out/'script_receiver_command_actual.h').write_text(pieces.replace('command_index','receiverCommandIndex').replace('error(','receiverSetupError(')+'\n')
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--root',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();extract(a.root,a.output)
