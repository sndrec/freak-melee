static unsigned long long shader_fingerprint=1469598103934665603ULL;
static void shader_number(unsigned value)
{for(unsigned b=0;b<4;++b){shader_fingerprint^=(value>>(b*8))&255;shader_fingerprint*=1099511628211ULL;}}
static void fingerprint_shader(const HSD_MObj* material)
{
    shader_number(0x544556);
    for(const HSD_TExpTevDesc* d=material->tevdesc;d;d=(const HSD_TExpTevDesc*)d->desc.next){
        shader_number(d->desc.flags);shader_number(d->desc.stage);shader_number(d->desc.color);
        // Texture pointers are process-local; preserve the assigned native ID.
        shader_number(d->tobj?d->tobj->id:~0u);
        const HSD_TevConf* c=&d->desc.u.tevconf;
#define FIELD(name) shader_number((unsigned)c->name)
        FIELD(clr_op);FIELD(clr_a);FIELD(clr_b);FIELD(clr_c);FIELD(clr_d);
        FIELD(clr_scale);FIELD(clr_bias);FIELD(clr_clamp);FIELD(clr_out_reg);
        FIELD(alpha_op);FIELD(alpha_a);FIELD(alpha_b);FIELD(alpha_c);FIELD(alpha_d);
        FIELD(alpha_scale);FIELD(alpha_bias);FIELD(alpha_clamp);FIELD(alpha_out_reg);
        FIELD(mode);FIELD(ras_swap);FIELD(tex_swap);FIELD(kcsel);FIELD(kasel);
#undef FIELD
    }
    shader_number(0x434e5354);
    for(const HSD_TExp* c=material->texp;c;c=c->comm.next){
        shader_number(c->cnst.comp);shader_number(c->cnst.ctype);shader_number(c->cnst.reg);
        shader_number(c->cnst.idx);shader_number(c->cnst.ref);
    }
    // Undefined descriptor padding and pre-setup texture coord/map slots are
    // deliberately excluded; all assigned numerical compiler fields are above.
}
