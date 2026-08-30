# Rutile task ledger

## Pending backend and runtime work

- [ ] `RT-VERSION-001`: Implement `rtVersion` in `rt-d3d12`, `rt-opengl`, and `rt-vulkan`. Each backend must return one `u64` packed as major, minor, patch, and snapshot with `RT_MAKE_VERSION`.
- [x] `RT-CORE-EXTENSIONS-001`: Update the runtime loader to resolve `RT_PROCEDURES` in one `rtLoad` operation. Built-in extensions do not require separate load calls.
- [ ] `RT-CORE-EXTENSIONS-002`: Update the logging and validation layers to forward and resolve the minimal core plus every built-in extension procedure inventory.
- [ ] `RT-PROGRAM-001`: Update and verify every backend for `Program` and `rtProgramSource(rt_program program, const char* entry_point, const u08* bytes, usize byte_size)`. Each backend must load every shader stage the RTSL Program binary exposes for the named entry point.
- [ ] `RT-PROGRAM-002`: Update and verify the logging and validation layers for the new Program procedure names and source arguments.
- [ ] `RT-SAMPLER-001`: Implement `rt_sampler`, its state setters, and `rtCmdBindSampler` in `rt-d3d12`, `rt-opengl`, and `rt-vulkan`.
- [ ] `RT-SAMPLER-002`: Update the logging and validation layers for sampler creation, state, destruction, and binding.
- [ ] `RT-EXT-TEXTURE-001`: Update the runtime loader, every backend, and both layers to use the single `RT_EXT_TEXTURE_PROCEDURES` inventory for textures, texture views, samplers, and their resource bindings.
- [ ] `RT-CORE-003`: Move `rtSetOutput` and `rtGetName` into the backend and layer core inventories, and remove `rtQueryFormatCapabilities` from every backend, layer, and loader.
