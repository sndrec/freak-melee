static unsigned throw_invincibility_cases_count;
static void throw_invincibility_cases(void)
{
    // MSL's bool was an int. Native C bool collapsed this countdown to one.
    // Exercise the unchanged actual fighter tick slice with integer storage.
    const int durations[]={0,1,2,20,90};
    for(unsigned duration=0;duration<5;++duration)for(unsigned other=0;other<4;++other)
        for(unsigned overlay=0;overlay<2;++overlay){
            Fighter fighter={0};fighter.throw_invincibility_frames=durations[duration];
            fighter.x198C=1;fighter.x1990=other&1;fighter.x2221_b0=(other&2)!=0;
            throw_overlay=overlay?9:11;throw_overlay_clears=0;
            for(int tick=0;tick<durations[duration]+2;++tick){
                advance_throw_invincibility(&fighter);
                int remaining=durations[duration]-tick-1;if(remaining<0)remaining=0;
                CHECK(fighter.throw_invincibility_frames==remaining);
                if(durations[duration]&&remaining==0){
                    CHECK(fighter.x198C==(other?2:0));
                    CHECK(throw_overlay_clears==overlay&&throw_overlay==(overlay?0:11));
                }else CHECK(fighter.x198C==1&&!throw_overlay_clears);
            }
            ++cases;++throw_invincibility_cases_count;
        }
    // The common grant preserves a longer duration, just like console int
    // storage did. No clamp, altered arithmetic or gameplay workaround.
    reset();throw_entry_case=true;Fighter_GObj* h=create(true);Fighter* f=h->user_data;
    f->throw_invincibility_frames=90;ftColl_8007B7A4(h,20);CHECK(f->throw_invincibility_frames==90);
    f->throw_invincibility_frames=2;ftColl_8007B7A4(h,20);CHECK(f->throw_invincibility_frames==20);
    HSD_GObjFree(h);++cases;++throw_invincibility_cases_count;reset();
}
