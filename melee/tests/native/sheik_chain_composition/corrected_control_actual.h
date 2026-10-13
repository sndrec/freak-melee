/* Actual typed native-layout repair reference extracted from
 * src/melee/it/kinds/itseakchain.c, root commit5859967ad.
 * Complete source SHA256: 7e1ab0ac697d4ecde7fbe3d69d296fb77789313b64836b6bcaf02cc59456189d
 * Frozen64 is preserved separately; only the common-union timer alias changed.
 */
void it_802BC080(ItemLink* link, Vec3* target, Item* ip)
{
    struct {
        Vec3 dir;
        u8 pad[16];
    } stack;
    f32 lstick_x, lstick_y;
    ItemLink* iter = link->prev;
    ItemLink* cur = link;
    itSeakChain_Attrs* attrs = ip->xC4_article_data->x4_specialAttributes;
    s32 last_idx = (s32) (0.5f * attrs->x0 - 1.0f);
    Fighter* fp;
    s32 env_flags;
    s32 use_arg = 0;
    s32 counter;
    s32 coll_arg;
    f32 scale;
    s32 mode;
    int i;
    PAD_STACK(12);

    while (iter != NULL && !cur->x2C_b0) {
        cur = iter;
        iter = iter->prev;
    }

    if (ip->xDD4_itemVar.seakchain.x18 != 0) {
        coll_arg = 1;
    } else {
        coll_arg = 0;
    }
    fp = GET_FIGHTER(ip->xDD4_itemVar.seakchain.parent_gobj);

    for (i = 0; last_idx > i; i++) {
        ip->xDD4_itemVar.seakchain.history[last_idx - i].x =
            ip->xDD4_itemVar.seakchain.history[last_idx - 1 - i].x;
        ip->xDD4_itemVar.seakchain.history[last_idx - i].y =
            ip->xDD4_itemVar.seakchain.history[last_idx - 1 - i].y;
    }

    if (ABS(fp->u.sk.lstick_delta.x) > attrs->x48) {
        s32 sign = (fp->u.sk.lstick_delta.x < 0.0f) ? -1 : 1;
        if (ip->facing_dir == sign) {
            ip->xDD4_itemVar.seakchain.history[0].x =
                fp->u.sk.lstick_delta.x * attrs->x38;
        } else {
            ip->xDD4_itemVar.seakchain.history[0].x =
                fp->u.sk.lstick_delta.x * attrs->x3C;
        }
    } else {
        ip->xDD4_itemVar.seakchain.history[0].x = 0.0f;
    }

    if (ABS(fp->u.sk.lstick_delta.y) > attrs->x48) {
        s32 sign = (fp->u.sk.lstick_delta.y < 0.0f) ? -1 : 1;
        if (sign > 0.0f) {
            ip->xDD4_itemVar.seakchain.history[0].y =
                fp->u.sk.lstick_delta.y * attrs->x40;
        } else {
            ip->xDD4_itemVar.seakchain.history[0].y =
                fp->u.sk.lstick_delta.y * attrs->x44;
        }
    } else {
        ip->xDD4_itemVar.seakchain.history[0].y = 0.0f;
    }

    // The retail common-union alias shared this offset with Sheik's rehit
    // timer. Native pointer width moves that alias to the resume timer instead.
    ip->xDD4_itemVar.seakchain.x18 = fp->mv.sk.specials.x1C;

    cur->vel.x += ip->xDD4_itemVar.seakchain.history[0].x * attrs->x1C;
    cur->vel.y += ip->xDD4_itemVar.seakchain.history[0].y * attrs->x20;
    itSeakChain_clamp_x10(cur, attrs);
    if (ABS(cur->vel.x) > attrs->x24) {
        if (cur->vel.x > 0.0f) {
            cur->vel.x = attrs->x24;
        } else {
            cur->vel.x = -attrs->x24;
        }
    }
    if (cur->vel.y > attrs->x18 - attrs->x28) {
        cur->vel.y -= attrs->x18;
    } else if (cur->vel.y < -attrs->x18 - attrs->x28) {
        cur->vel.y += attrs->x18;
    }
    if (ABS(cur->vel.y) > attrs->x2C) {
        if (cur->vel.y > 0.0f) {
            cur->vel.y = attrs->x2C;
        } else {
            cur->vel.y = -attrs->x2C;
        }
    }

    it_802A4420(cur);

    scale = 1.0f;
    scale *= attrs->x34;
    if (it_802A3C98(&cur->pos, target, &stack.dir) > attrs->x4) {
        cur->pos.x = stack.dir.x * attrs->x4 + target->x;
        cur->pos.y = stack.dir.y * attrs->x4 + target->y;
        cur->pos.z = stack.dir.z * attrs->x4 + target->z;
    }

    use_arg = ip->xDD4_itemVar.seakchain.x10;
    lstick_x = ftSk_SpecialS_80110F58(ip->xDD4_itemVar.seakchain.parent_gobj);
    lstick_y = ftSk_SpecialS_80110F64(ip->xDD4_itemVar.seakchain.parent_gobj);
    if (ABS(lstick_x) < attrs->x48 && ABS(lstick_y) < attrs->x48) {
        mode = 3;
    } else if (lstick_y < -0.5f) {
        mode = 1;
    } else {
        mode = 2;
    }

    counter = 0;
    while (iter != NULL) {
        f32 dx, dy;
        s32 idx = (s32) (0.5f * (counter + 1));
        f32 fx = attrs->x1C;
        f32 fy = attrs->x20;
        dx = scale * (ip->xDD4_itemVar.seakchain.history[idx].x * fx);
        dy = scale * (ip->xDD4_itemVar.seakchain.history[idx].y * fy);
        if (use_arg != 0) {
            dx *= attrs->x30;
        }
        iter->vel.x += dx;
        iter->vel.y += dy;

        {
            f32 lim = attrs->x10 * scale;
            if (iter->vel.x > lim) {
                iter->vel.x -= lim;
            } else if (iter->vel.x < -attrs->x10 * scale) {
                iter->vel.x += lim;
            } else {
                iter->vel.x = 0.0f;
            }
        }
        {
            if (ABS(iter->vel.x) > attrs->x24 * scale) {
                if (iter->vel.x > 0.0f) {
                    iter->vel.x = attrs->x24 * scale;
                } else {
                    iter->vel.x = -attrs->x24 * scale;
                }
            }
        }
        {
            f32 vy_lim = attrs->x18 * scale;
            if (iter->vel.y > vy_lim - attrs->x28) {
                iter->vel.y -= vy_lim;
            } else if (iter->vel.y < -attrs->x18 * scale - attrs->x28) {
                iter->vel.y += vy_lim;
            }
        }
        {
            if (ABS(iter->vel.y) > attrs->x2C * scale) {
                if (iter->vel.y > 0.0f) {
                    iter->vel.y = attrs->x2C * scale;
                } else {
                    iter->vel.y = -attrs->x2C * scale;
                }
            }
        }

        counter++;
        scale *= attrs->x34;
        it_802A4420(iter);

        if (counter > mode) {
            if (counter % 5 == 0) {
                env_flags = it_802BBAEC(iter, coll_arg, attrs->x4);
            } else {
                env_flags = it_802BBAEC(iter, 0, attrs->x4);
            }
        } else {
            env_flags = 0;
        }
        env_flags &= 0x18000;
        if (env_flags != 0) {
            if (ABS(iter->vel.y) > attrs->x60) {
                iter->vel.y *= -attrs->x5C;
            } else {
                itSeakChain_clamp_x14(iter, attrs);
                iter->vel.y = 0.0f;
            }
        }

        if (it_802A3C98(&iter->pos, &cur->pos, &stack.dir) > attrs->x4) {
            iter->pos.x = stack.dir.x * attrs->x4 + cur->pos.x;
            iter->pos.y = stack.dir.y * attrs->x4 + cur->pos.y;
            iter->pos.z = stack.dir.z * attrs->x4 + cur->pos.z;
        }

        cur = iter;
        iter = iter->prev;
    }
    ip->xDD4_itemVar.seakchain.x10 = env_flags;
}
