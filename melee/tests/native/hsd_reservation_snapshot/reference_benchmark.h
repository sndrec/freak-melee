#ifdef NATIVE_SNAPSHOT_BENCHMARK
#include <time.h>
// Controlled owner-fixture cost, not whole-match or rollback acceptance.
static void benchmark_references(HSD_JObj* joint,HSD_Particle* particle,
                                 NativeParticleControlSnapshot* snapshot)
{
    const unsigned queries=1000000,captures=100000;
    const unsigned before_checks=checks;
    clock_t start=clock();
    for(unsigned i=0;i<queries;++i) {
        NativeClassReference reference;
        CHECK((reference=nativeClass_Reference(joint,sizeof(*joint))).bytes);
        CHECK(nativeClass_Resolve(reference)==joint);
    }
    double class_us=1e6*(double)(clock()-start)/CLOCKS_PER_SEC/queries;
    start=clock();
    for(unsigned i=0;i<queries;++i) {
        NativeObjAllocReference reference;
        CHECK((reference=nativeObjAlloc_Reference(&hsd_804D0F60.alloc_data,particle,sizeof(*particle))).epoch);
        CHECK(nativeObjAlloc_Resolve(reference)==particle);
    }
    double ordinary_us=1e6*(double)(clock()-start)/CLOCKS_PER_SEC/queries;
    start=clock();
    for(unsigned i=0;i<captures;++i) {
        size_t bytes;
        CHECK(nativeParticleControl_Save(snapshot,sizeof(*snapshot),&bytes));
        CHECK(bytes==sizeof(*snapshot));
    }
    double capture_us=1e6*(double)(clock()-start)/CLOCKS_PER_SEC/captures;
    checks=before_checks; // Keep timing-loop assertions out of coverage totals.
    fprintf(stderr,"reference-benchmark queries=%u captures=%u class_us=%.3f ordinary_us=%.3f particle_save_us=%.3f\n",
            queries,captures,class_us,ordinary_us,capture_us);
}
#endif
