#define THROW_ANGLE_SOURCE \
 "func ThrowAngle():\n" \
 "    let float muzzle_x = 0\n    let float muzzle_y = 0\n    let float muzzle_z = 0\n" \
 "    let float barrel_x = 0\n    let float barrel_y = 0\n    let float barrel_z = 0\n" \
 "    bones.world_position_offset(bones.index(RightThumb), muzzle_x, muzzle_y, muzzle_z, 0, 1.2325000762939453, 4.263599872589111)\n" \
 "    bones.world_position_offset(bones.index(RightThumb), barrel_x, barrel_y, barrel_z, 0, 1.2325000762939453, 0.013600001111626625)\n" \
 "    state.x = atan2_float(muzzle_y - barrel_y, muzzle_x - barrel_x)\n"
