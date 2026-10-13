# Spatial scripting contracts

`native_script_spatial_compiler` compiles fighter explicit-position wind and
article joint world-position queries, including Float-local and scalar-state
destinations. It rejects wrong argument types, invalid graph paths, aliasing,
undeclared outputs and use of the article query in fighter context. Generated
Host requirements include the actual operations used.

`native_script_spatial_module` executes generated callbacks with controlled
query/wind/RNG boundaries. It verifies missing Host rejection, one evaluation
per argument in written order, primary/extra graph selection, output stores and
defined missing-joint zeros. It does not claim actual world matrices or rendering.

The existing `native_model_runtime` test separately executes production
`nativeArticleModel_ReadWorldPosition`, real graph resolution and the shared
`lb_8000B1CC` operation for primary root/children and an extra model. Invalid
model/path/source/output cases refuse the query. Comparison is against the
same stock world-position operation on the explicitly selected joint.
