bool ftCommon_80080144(Fighter* fp)
{
    int kind = fp->kind;
    if ((kind == Ft_Kind_Popo || kind == Ft_Kind_Nana) &&
        fp->x619_costume_id >= 2)
    {
        return true;
    }
    return false;
}

s32 original_ft_80087D0C(Fighter* fighter, s32 sfx_id)
{
    enum_t sfx = lbAudioAx_800233EC(sfx_id);
    enum_t ssm_id = lbAudioAx_80023130(sfx);

    switch (ssm_id) {
    case 0:
        switch (sfx) {
        case 407:
        case 467:
        case 503:
        case 515:
        case 290051:
        case 509:
        case 485:
        case 491:
        case 479:
        case 473:
        case 437:
        case 455:
        case 461:
        case 449:
        case 443:
        case 425:
        case 431:
        case 419:
        case 413:
        case 377:
        case 395:
        case 401:
        case 389:
        case 383:
        case 371:
            sfx_id = ft_80087C70(fighter, sfx);
            if (fighter->is_metal) {
                sfx_id += 3;
            }
            sfx = sfx_id;
            break;
        case 359:
        case 368:
        case 365:
        case 362:
        case 344:
        case 353:
        case 356:
        case 350:
        case 347:
        case 338:
        case 341:
        case 335:
        case 332:
            sfx = ft_80087C70(fighter, sfx);
            break;
        }
        break;
    case 13: {
        switch (fighter->kind) {
        case Ft_Kind_Popo:
        case Ft_Kind_Nana:
            if (0x1FBFD <= sfx && sfx <= 0x1FC62 &&
                ftCommon_80080144(fighter) == true)
            {
                sfx += 0x66;
            }

            if (0x1FC63 <= sfx && sfx <= 0x1FCC8 &&
                ftCommon_80080144(fighter) != true)
            {
                sfx -= 0x66;
            }
            break;
        default:
            break;
        }
    }
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 14:
    case 15:
    case 16:
    case 17:
    case 18:
    case 19:
    case 20:
    case 21:
    case 22:
    case 23:
    case 24:
    case 25:
    case 26:
    case 27:
    case 28:
    case 29:
    case 30:
    case 31:
    case 33: {
        int sfx_offset;
        int unused_output;
        int _[1];

        if (lbAudioAx_800230C8(ssm_id, &sfx_offset, &unused_output) == 0 &&
            sfx >= sfx_offset + lbAudioAx_80023220(ssm_id))
        {
            sfx = ft_80087C70(fighter, sfx);
        }
        break;
    }
    default:
        break;
    }

    return sfx;
}

s32 original_ftLib_8008746C(HSD_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);

    if (ftCommon_80080144(fp)) {
        return 0x1FBD1;
    }

    return fp->ft_data->x4C_sfx->x34;
}

float original_ft_GetGroundFrictionMultiplier(Fighter* fp)
{
    if (fp->kind == Ft_Kind_Popo || fp->kind == Ft_Kind_Nana) {
        return 1.0F;
    }
    return mpColl_8004CA6C(&fp->coll_data);
}
