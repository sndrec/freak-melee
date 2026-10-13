/* src/melee/ft/ft_08A1.c:73 */
void ft_8008A348(Fighter_GObj* gobj, float anim_blend)
{
    Fighter* fp = GET_FIGHTER(gobj);
#ifdef MELEE_NATIVE
    uint32_t generation;
    if (!nativeCallbackSource_Snapshot(gobj, NativeCallbackSource_Fighter, &generation)) return;
    const FighterKind source_kind = fp->kind;
    const NativeFighterDefinition* definition = nativeFighterRuntime_Definition(source_kind);
#endif
    if (fp->x2224_b2) {
        ftCo_DownSpot_Enter(gobj);
        return;
    }
    if (ftCo_800C5240(gobj)) {
        ftCo_800C4ED8(gobj);
        return;
    }
    if (fp->ground_or_air == GA_Air) {
        ftCommon_8007D7FC(fp);
    }
    if (fp->kind == Ft_Kind_Peach) {
        if (fp->item_gobj != NULL &&
            itGetKind(fp->item_gobj) == It_Kind_Peach_Parasol)
        {
            it_802BDB94(fp->item_gobj);
        }
    }
    Fighter_ChangeMotionState(gobj, ftCo_MS_Wait, Ft_MF_None, 0.0f, 1.0f,
                              anim_blend, NULL);
#ifdef MELEE_NATIVE
    if (!nativeCallbackSource_Alive(gobj, generation, NativeCallbackSource_Fighter) ||
        fp->kind != source_kind || nativeFighterRuntime_Definition(fp->kind) != definition) return;
#endif
    if (ftCo_8008A698(fp) && ftData_80085FD4(fp, ftCo_MS_DeadUpFall)->x8 != 0)
    {
        ftCo_8008A6D8(gobj, ftCo_MS_DeadUpFall);
    }
#ifdef MELEE_NATIVE
    if (!nativeCallbackSource_Alive(gobj, generation, NativeCallbackSource_Fighter) ||
        fp->kind != source_kind || nativeFighterRuntime_Definition(fp->kind) != definition) return;
#endif
    ftCommon_8007EFC0(fp, p_ftCommonData->x5F0);
#ifdef MELEE_NATIVE
    if (definition != NULL && definition->on_wait_enter != NULL) {
        definition->on_wait_enter(gobj);
        return;
    }
#endif
    switch (fp->kind) {
    case Ft_Kind_Link:
        ftLk_AttackAir_800EB3BC(gobj);
        return;
    case Ft_Kind_CLink:
        ftCl_Init_8014919C(gobj);
        return;
    default:
        return;
    }
}

/* src/melee/ft/ft_0D27.c:70 */
void fn_800D2890(Fighter_GObj* gobj, int ms_id)
{
    Fighter* fp = GET_FIGHTER(gobj);
    if (fp->kind == Ft_Kind_Peach && ms_id == 0x172) {
        fp->mv.co.mushroom.resume_air = ftCo_800968C8;
        return;
    }
    switch (ms_id) {
    case ftCo_MS_FallAerial:
        fp->mv.co.mushroom.resume_air = ftCo_FallAerial_Enter;
        return;
    case ftCo_MS_FallSpecial:
    case ftCo_MS_ItemParasolFallSpecial:
        fp->mv.co.mushroom.resume_air = ftCo_800968C8;
        return;
    default:
        fp->mv.co.mushroom.resume_air = ftCo_Fall_Enter;
        return;
    }
}

/* src/melee/ft/kinds/ftCommon/ftCo_Landing.c:39 */
void ftCo_Landing_Enter(Fighter_GObj* gobj, FtMotionId msid,
                        bool allow_interrupt, MotionFlags flags,
                        float anim_start, float anim_speed)
{
    Fighter* fp = GET_FIGHTER(gobj);

    ftCommon_8007D7FC(fp);
    Fighter_ChangeMotionState(gobj, msid, flags, anim_start, anim_speed, 0.0F,
                              NULL);
    fp->mv.co.landing.allow_interrupt = allow_interrupt;
#ifdef MELEE_NATIVE
    switch (fp->behavior_kind) {
#else
    switch (fp->kind) {
#endif
    case Ft_Kind_Mario:
    case Ft_Kind_DrMario:
        fp->u.mr.x2234_tornadoCharge = false;
        fp->u.mr.x2238_isCapeBoost = false;
        break;
    case Ft_Kind_Peach:
        fp->u.pe.specialairn_used = false;
        if (allow_interrupt == true) {
            ftPe_8011D598(gobj);
        }
    default:
        break;
    case Ft_Kind_Mars:
    case Ft_Kind_Emblem:
        fp->u.ms.x222C = 0;
        break;
    case Ft_Kind_GameWatch:
        fp->u.gw.x2234 = 0;
        break;
    case Ft_Kind_Popo:
    case Ft_Kind_Nana:
        fp->u.pp.x224C = 0;
        break;
    case Ft_Kind_Kirby:
        fp->u.kb.xCC = false;
        fp->u.kb.xC4 = false;
        fp->u.kb.x64 = false;
        break;
    case Ft_Kind_Mewtwo:
        fp->u.mt.x223C_isConfusionBoost = false;
        break;
    }
#ifdef MELEE_NATIVE
    const NativeFighterDefinition* definition =
        nativeFighterRuntime_Definition(fp->kind);
    if (definition != NULL && definition->on_landing_action != NULL)
        definition->on_landing_action(gobj);
#endif
}

/* src/melee/ft/fighter.c:1562 */
void Fighter_8006A360(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    const uint32_t callback_generation = (uint32_t)fp->x8_spawnNum;

    if (!fp->x221F_b3) {
        fp->pos_delta.x = fp->cur_pos.x - fp->prev_pos.x;
        fp->pos_delta.y = fp->cur_pos.y - fp->prev_pos.y;
        fp->pos_delta.z = fp->cur_pos.z - fp->prev_pos.z;

        fp->prev_pos = fp->cur_pos;

        if (fp->dmg.x18C8 != -1 && fp->dmg.x18C8 > 0) {
            fp->dmg.x18C8--;
            if (fp->dmg.x18C8 == 0) {
                fp->dmg.x18c4_source_ply = 6;
                fp->dmg.x18C8 = -1;
                fp->dmg.x18D0 = -0xA;
            }
        }

        if (fp->x1990) {
            fp->x1990--;
            if (fp->x1990 == 0 && !fp->x2221_b0) {
                fp->x198C = fp->x1994 != 0 ? 1 : 0;

                if (ftCo_800C0694(fp) == 9) {
                    ftCo_800C0200(fp, 9);
                }
            }
        }

        if (fp->x1994) {
            fp->x1994--;
            if (fp->x1994 == 0) {
                fp->x198C = (fp->x2221_b0 || fp->x1990 != 0) ? 2 : 0;

                if (ftCo_800C0694(fp) == 9) {
                    ftCo_800C0200(fp, 9);
                }
            }
        }

        if (fp->x221D_b6) {
            if (fp->x2004) {
                fp->x2004--;
                if (fp->x2004 == 0) {
                    fp->x221D_b6 = 0;
                    if (ftCo_800C0694(fp) == 0x6B) {
                        ftCo_800C0200(fp, 0x6B);
                    }
                } else if (fp->x2004 == it_8026B588()) {
                    ft_800880D8(fp);
                }
            }
        }

        if (fp->x2220_b5 || fp->x2220_b6) {
            if (fp->x2008) {
                fp->x2008--;
            }

            if (fp->x2008 == 0) {
                if (fp->x2220_b5) {
                    Fighter_SuperMushroomEnd(gobj);
                } else if (fp->x2220_b6) {
                    Fighter_PoisonMushroomEnd(gobj);
                }
            }
        }

        if (fp->x197C) {
            if (fp->x2014) {
                fp->x2014--;
                if (fp->x2014 == 0) {
                    ftCommon_8007F8E8(gobj);
                    if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
                    Item_8026A8EC(fp->x197C);
                    if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
                    ftCommon_8007F9B4(gobj);
                }
            }
        }

        if (fp->is_metal && fp->metal_timer != 0) {
            fp->metal_timer--;
            if (fp->metal_timer == 0 || fp->metal_health <= 0) {
                // Exit fighter metal mode
                ftCo_800C8540(gobj);
            }
        }

        if (fp->x2227_b3) {
            if (fp->x2034) {
                fp->x2034--;
                if (!fp->x2034 || fp->x2038 <= 0) {
                    ftCo_800C9034(gobj);
                    return;
                }
            }
        }

        if (fp->x1980) {
            fp->x2018--;
            fp->x201C--;

            if (fp->x201C == 0) {
                Fighter_TakeDamage_8006CC7C(fp,
                                            p_ftCommonData->x6F4_unkDamage);
                fp->x201C = p_ftCommonData->x6F8;
            }

            if (fp->x2018 <= 0) {
                Vec3 vec = Fighter_803B7488;

                ftCommon_8007F8E8(gobj);
                if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
                Item_8026ABD8(fp->x1980, &vec, 0.0f);
                if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
                ftCommon_8007FF74(gobj);
            }
        }

        if (fp->x2226_b4) {
            if (fp->x2030) {
                fp->x2030--;
                if (fp->x2030 == 0) {
                    ftCo_800C8A64(gobj);
                    return;
                }
                if (!fp->x2226_b3 && fp->x2030 == p_ftCommonData->x7D0 &&
                    ftCo_800C8B2C(fp, 0x7D, 0))
                {
                    fp->x2226_b7 = 1;
                }
            }
        }

        if (fp->x2221_b4) {
            if (fp->x2104) {
                fp->x2104--;
                if (fp->x2104 == 0) {
                    fp->x2221_b4 = false;

                    if (fp->item_gobj &&
                        itGetKind(fp->item_gobj) == It_Kind_Peach_Parasol)
                    {
                        fp->x2221_b5 = true;
                        ftCo_800968C8(gobj);
                    } else {
                        ftCo_80095744(gobj, 0);
                    }
                }
            }
        }

        if (!fp->x221F_b4 && Camera_80031144() == 1.0f) {
            if (fp->dmg.x1830_percent < p_ftCommonData->x7B0) {
                if (ifMagnify_802FC998(fp->player_id) &&
                    (Player_GetMoreFlagsBit3(fp->player_id) != 0))
                {
                    fp->dmg.x1910++;
                } else {
                    fp->dmg.x1910 = 0;
                }

                if (fp->dmg.x1910 >= p_ftCommonData->x7AC) {
                    Fighter_TakeDamage_8006CC7C(
                        fp, p_ftCommonData->x7B4_unkDamage);
                    fp->dmg.x1910 = 0;
                }
            }
        }

        if (fp->dmg.x18F0) {
            fp->dmg.x18F0--;
            if (fp->dmg.x1830_percent > 0.0f) {
                fp->dmg.x1830_percent--;
                ft_80088640(fp, 0x7D, 0x7F, 0x40);
                Player_SetHPByIndex(fp->player_id, fp->x221F_b4,
                                    fp->dmg.x1830_percent);
                pl_80040B8C(fp->player_id, fp->x221F_b4, 1);
            }

            if (fp->dmg.x1830_percent <= 0.0f) {
                fp->dmg.x1830_percent = 0.0f;
                fp->dmg.x18F0 = 0;
            }

            if (fp->dmg.x18F0 == 0) {
                if (ftCo_800C0694(fp) == 8) {
                    ftCo_800C0200(fp, 8);
                }
                ftCommon_8007ECD4(fp, 2);
            }
        }

        if (fp->item_gobj) {
            if (itGetKind(fp->item_gobj) != It_Kind_Hammer) {
                !fp;
            } else {
                ftCo_800C511C(gobj);
            }
        }

        if (fp->dmg.x18fa_model_shift_frames) {
            fp->dmg.x18fa_model_shift_frames--;
            fp->dmg.x18FC++;
            if (fp->dmg.x18FC >= fp->dmg.x18FD) {
                fp->dmg.x18FC = 0;
            }
        }

        if (ftData_UnkMotionStates3[fp->kind]) {
            ftData_UnkMotionStates3[fp->kind](gobj);
            if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
        }

        if (fp->x21CC) {
            fp->x21CC(gobj);
            if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
        }

        if (!fp->x2219_b5) {
            if (fp->x209A > 1 && !fp->x221D_b4) {
                fp->x209A--;
            }
            if (fp->x2223_b0) {
                if (fp->x104 == 0x14U) {
                    ftAnim_8006F0FC(gobj, 0.0f);
                } else {
                    fp->frame_speed_mul += fp->x8A0_unk;
                }
                fp->x104--;
                if (fp->x104 == 0) {
                    ftAnim_8006F0FC(gobj, fp->frame_speed_mul);
                    fp->x104 = 0x14U;
                }
            }

            if (fp->smash_attrs.x2138_smashSinceHitbox != -1.0f) {
                fp->smash_attrs.x2138_smashSinceHitbox++;
            }

            if (fp->dmg.x18ac_time_since_hit != -1) {
                fp->dmg.x18ac_time_since_hit++;
            }
            ftAnim_8006EBA4(gobj);
            ftCo_800D71D8(gobj);
            ftColl_800764DC(gobj);

            if (!fp->x221C_b6) {
                pl_800411C4(fp->player_id, fp->x221F_b4);
            }
            ftCo_800DEF38(gobj);

            if (fp->anim_cb) {
                fp->anim_cb(gobj);
                if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
            }
        }

        ftCommon_8007E0E4(gobj);
        ftCo_800C0408(gobj);
    }
}
