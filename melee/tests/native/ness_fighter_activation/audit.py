"""Bounded source census beyond special motion-phase ownership."""
from pathlib import Path
import hashlib,json


def digest(p): return hashlib.sha256(p.read_bytes()).hexdigest()


def audit(snapshot,output,candidate):
    sdk=snapshot/'sdk'
    groups={
      'stock_callback_tables': ['src/melee/ft/ftdata.c','src/melee/ft/kinds/ftNess/ftness.c'],
      'shared_smash_entries': ['src/melee/ft/kinds/ftCommon/ftCo_AttackS4.c','src/melee/ft/kinds/ftCommon/ftCo_AttackHi4.c','src/melee/ft/kinds/ftCommon/ftCo_AttackLw4.c'],
      'jump_and_worn_item': ['src/melee/ft/kinds/ftCommon/ftCo_JumpAerial.c','src/melee/ft/kinds/ftCommon/ftCo_ItemScrew.c','src/melee/ft/kinds/ftCommon/ftCo_JumpAerialF1.c'],
      'cpu': ['src/melee/ft/ftcpuattack.c','src/melee/ft/kinds/ftCommon/ftCo_0A01.c'],
      'incapacitated_item': ['src/melee/ft/ft_0BEC.c','src/melee/ft/ft_0BF0.c'],
      'animation_and_model_policy': ['src/melee/ft/ft_0892.c','src/melee/ft/ftwaitanim.c','src/melee/ft/ftparts.c','src/melee/ft/ftdynamics.c','src/melee/ft/ftdemo.c','src/melee/ft/fighter.c'],
      'effects': ['src/melee/ef/efsync.c','src/melee/ef/efasync.c','src/melee/ef/eflib.c'],
      'resource_and_registration': ['src/melee/ft/kinds/ftNess/ftness_clone.c','src/melee/ft/native_fighter_runtime.c','src/melee/ft/native_fighter_module.c','src/melee/ft/native_fighter_article_bindings.c','src/melee/it/itarticle_data.c'],
    }
    hashes={name:{path:digest(sdk/path) for path in paths} for name,paths in groups.items()}
    verdicts={
      'load_reset':'owned copy/reload; native -1 initialization; death hides group0 and forgets four references',
      'damage':'one owned YoYo->Flash->Thunder->Bat chain; no mixed private-union callback',
      'item_notifications':'both pickup/drop pairs, visibility, knockback use owned selectors; retained/auxiliary references use common engine lifecycle',
      'smash':'three full entry hooks plus seven states own all 28 phases and resource callbacks; intentional timed-rehit cleanup correction retained',
      'specials':'eight authored entries and all four specials complete; six empty original phases owned; original two NULL Fire IASA slots explicitly no-op',
      'jump':'owned aerial jump entry and physics; worn sound false at common item-induced/multijump routes',
      'cpu':'owned attack/travel/recovery/candidate/projectile-hit; other Ness-specific defense/capture/ranged branches absent',
      'presentation':'owned star-ice/camera-ice/sleep Bat; ordinary KO setup shared; down/right paths deliberately create none',
      'animation_metadata':'Ness has no special action-metadata/status/idle/angled-motion exceptions; common policies remain common',
      'dynamics_and_model':'Ness is absent from special wind/interaction/hat branches; both stock extra model callback entries NULL',
      'effects':'1262/1263 joint-follow models,1264 asynchronous attachment,1030 rebound presentation; no Ness union/kind updater found in effect sources',
      'assets':'own rig51/63; immutable article mask0x7ff, Yoyo serialized layout, 11 slots, sound bank21/effect bank10; animation family Ness is a retargeting tag, not behavior inheritance',
      'demo':'copied results/intro/ending/viwait root strings and clips; no Ness extra demo callback; rendered results acceptance pending',
      'foreign_helper':'Sheik calls ftNs_AttackHi4_YoyoCheckEnvColl as a generic collision utility; not a Ness callback or inheritance path',
      'kirby':'copy behavior deferred; does not become required behavior of this standalone Ness',
    }
    generated=(output/'project/behavior.generated.c').read_text()
    for forbidden in ['fighter->u.ns','itemVar.ness','ftNs_','it_802AD478','it_802BE9D8']:
      if forbidden in generated: raise RuntimeError('Unexpected stock/union generated reference: '+forbidden)
    paths={
      'examples/ness_wip.fms':'Replace only after drawn acceptance with generated full candidate and accurate completion comments; preserve accepted historical source ledgers.',
      'src/melee/ui/native_shell_workshop_editor.cpp':'No change: Ft_Kind_Ness already selects ness_wip.fms.',
      'CMakeLists.txt':'No change: examples/ness_wip.fms is already copied to ui.',
      'src/melee/ft/kinds/ftNess/ftness_clone.c':'No change: copied metadata/rig11 assets correct; keep raw catalog source_kind=Ness for untouched clone fallback.',
      'src/melee/ft/native_fighter_runtime.c':'No change: generated module None accepted against raw base Ness; BehaviorKind then returns active custom kind.',
      'src/melee/ft/native_fighter_article_bindings.c':'No change: all11 article slots already registered.',
    }
    result={'snapshot':str(snapshot),'source_groups':hashes,'decisions':verdicts,
      'remaining_generic_contract_gaps':[], 'candidate_sha256':digest(candidate),
      'activation_changes':paths,'limits':['No game, pixels/audio, Workshop interaction or replay/rollback/network execution has been accepted here.','Four borrowed animation clips remain immutable clip dependencies, not stock gameplay callback pointers.','Retains bounded binary32 CPU endpoint judgments and intentional yo-yo cleanup delta from prior source acceptance.']}
    (output/'audit.json').write_text(json.dumps(result,indent=2)+'\n')
    return result
