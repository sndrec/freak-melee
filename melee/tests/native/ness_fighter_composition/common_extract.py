"""Unchanged common bodies, pinned separately from the generated candidate SDK."""
from pathlib import Path
from run import extract, sha


def prepare(reference, snapshot, output):
    groups = {
        'jump': {
            'src/melee/ft/kinds/ftCommon/ftCo_JumpAerial.c': [
                'ft_SetVec', 'ftCo_JumpAerial_Begin', 'ftNs_JumpAerial_Enter',
                'ftNs_JumpAerial_Phys_Cb'],
            'src/melee/ft/ftmovement.c': ['ftCommon_CalcSelfAccel_Deaccel',
                'ftCommon_CalcSelfAccel_AccelToVelClampedFrom',
                'ftCommon_CalcSelfAccel_DriftFrom'],
            'src/melee/ft/ft_084E.c': ['ft_800851D0'],
        },
        'presentation': {
            'src/melee/ft/ft_0BEC.c': ['setupInitialState', 'FoxHelper', 'DocHelper',
                'ftCo_800BECB0', 'ftCo_800BED88', 'ftCo_800BEF04'],
            'src/melee/ft/ft_0BF0.c': ['ftCo_800BF034', 'ftCo_800BF108'],
        },
        'projectile': {
            'src/melee/ft/ftcpuattack.c': ['ftCo_CpuPredictHitboxPosition', 'ftCo_800BB220'],
        },
        'candidate': {
            'src/melee/ft/kinds/ftCommon/ftCo_0A01.c': ['ftCo_800A2C08'],
            'src/melee/ft/ftcpuattack.c': ['ftCo_800B9790'],
        },
    }
    inputs = {}
    for group, paths in groups.items():
        for label, root in [('original', reference), ('current', snapshot)]:
            if group == 'candidate' and label == 'current':
                continue
            body = ''.join(extract((root/'sdk'/path).read_text(), name)
                for path, names in paths.items() for name in names)
            (output/(group+'_'+label+'.h')).write_text(body, encoding='utf-8')
            inputs[group+'_'+label] = {path: sha(root/'sdk'/path) for path in paths}
    random_path = 'src/sysdolphin/baselib/random.c'
    text = (reference/'sdk'/random_path).read_text()
    (output/'random_original.h').write_text('u32 seed=1; u32* seed_ptr=&seed;\n'+
        extract(text, 'HSD_Randf'), encoding='utf-8')
    inputs['random_original'] = {random_path: sha(reference/'sdk'/random_path)}
    path = 'src/melee/it/kinds/itnessbat.c'
    (output/'presentation_bat_original.h').write_text(
        extract((reference/'sdk'/path).read_text(), 'it_802AD590') +
        extract((reference/'sdk'/path).read_text(), 'it_2725_Logic58_PickedUp'), encoding='utf-8')
    inputs['presentation_bat_original'] = {path: sha(reference/'sdk'/path)}
    path = 'src/melee/ft/kinds/ftNess/ftness.c'
    names = ['ftNs_Init_OnLoad','ftNs_Init_LoadSpecialAttrs','ftNs_Init_OnItemPickup',
             'ftNs_Init_OnItemDrop','ftNs_Init_OnItemInvisible','ftNs_Init_OnItemVisible',
             'ftNs_Init_OnKnockbackEnter','ftNs_Init_OnKnockbackExit']
    (output/'notifications_original.h').write_text(''.join(
        extract((reference/'sdk'/path).read_text(), n) for n in names),encoding='utf-8')
    inputs['notifications_original'] = {path: sha(reference/'sdk'/path)}
    path = 'src/melee/ft/inlines.h'
    text = (reference/'sdk'/path).read_text()
    start=text.index('#define PUSH_ATTRS')
    end=text.index('#ifdef M2C',start)
    helpers=['Fighter_OnItemPickup','Fighter_OnItemDrop','Fighter_OnItemInvisible',
             'Fighter_OnItemVisible','Fighter_OnKnockbackEnter','Fighter_OnKnockbackExit']
    # fighter.h already includes the actual shared inline primitives. Require
    # their old reference bodies to be unchanged rather than define a second
    # function or redirect them through a fixture wrapper.
    current=(snapshot/'sdk'/path).read_text()
    if any(extract(text,n)!=extract(current,n) for n in helpers):
        raise RuntimeError('Notification primitive changed; add a true two-body comparison')
    (output/'notification_primitives.h').write_text(text[start:end],encoding='utf-8')
    inputs['notification_primitives'] = {path: sha(reference/'sdk'/path)}
    path='src/melee/ft/native_fighter_module.c'
    (output/'attributes_current.h').write_text(''.join(
        extract((snapshot/'sdk'/path).read_text(),n) for n in
        ['copy_ext_attributes','reload_ext_attributes']),encoding='utf-8')
    inputs['attributes_current']={path:sha(snapshot/'sdk'/path)}
    empty = {
        'src/melee/ft/kinds/ftNess/ftnessspecialn.c': ['ftNs_SpecialNStart_IASA','ftNs_SpecialAirNStart_IASA'],
        'src/melee/ft/kinds/ftNess/ftnessspecialhi.c': ['ftNs_SpecialHiStart_IASA','ftNs_SpecialAirHiStart_IASA'],
        'src/melee/ft/kinds/ftNess/ftnessspeciallw.c': ['ftNs_SpecialLwStart_IASA','ftNs_SpecialAirLwStart_IASA'],
    }
    (output/'empty_original.h').write_text(''.join(extract((reference/'sdk'/path).read_text(),n)
        for path,names in empty.items() for n in names),encoding='utf-8')
    inputs['empty_original']={p:sha(reference/'sdk'/p) for p in empty}
    path='src/melee/it/kinds/inlines.h'
    helpers=''.join(extract((reference/'sdk'/path).read_text(),n) for n in
        ['Item_InitSpawn','Item_InitSpawnOnPlaneNoInitialCollision'])
    bat_path='src/melee/it/kinds/itnessbat.c'
    yoyo_path='src/melee/it/kinds/itnessyoyo.c'
    yoyo=extract((reference/'sdk'/yoyo_path).read_text(),'it_802BE9D8')
    # Exact original prefix through allocation, with an explicit fixture return.
    # Successful YoYo initialization is covered by its separate accepted suite.
    allocation='    gobj = Item_80268B18(&spawn);'
    yoyo=yoyo[:yoyo.index(allocation)+len(allocation)]+'\n    return gobj;\n}\n'
    generic_path='src/melee/it/native_article.c'
    generic=(snapshot/'sdk'/generic_path).read_text()
    (output/'constructors_original.h').write_text(helpers+
        extract((reference/'sdk'/bat_path).read_text(),'it_802AD478')+yoyo,
        encoding='utf-8')
    origin_paths={'src/melee/ft/ftlib.c':['vector_add','ftLib_80086990'],
                  'src/melee/it/it_26B1.c':['it_8026BB68']}
    origin=''.join(extract((snapshot/'sdk'/p).read_text(),n)
        for p,names in origin_paths.items() for n in names)
    (output/'constructor_current.h').write_text(origin+extract(generic,'article_logic')+
        extract(generic,'nativeArticle_Create'),encoding='utf-8')
    inputs['constructor_original']={p:sha(reference/'sdk'/p) for p in [path,bat_path,yoyo_path]}
    inputs['constructor_current']={generic_path:sha(snapshot/'sdk'/generic_path)}
    inputs['constructor_current'].update({p:sha(snapshot/'sdk'/p) for p in origin_paths})
    return inputs
