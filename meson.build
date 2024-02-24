project(
    'c_tinywl', 
    'c', 
    version: '0.1.0', 
    license: 'MIT', 
    meson_version: '>=1.3.0',
    default_options: [
        'c_std=c11',
        'warning_level=2'
    ]
)

cc = meson.get_compiler('c')
add_project_arguments(cc.get_supported_arguments([
    '-DWLR_USE_UNSTABLE',

    '-Wundef',
    '-Wlogical-op',
    '-Wmissing-include-dirs',
    '-Wold-style-definition',
    '-Wpointer-arith',
    '-Winit-self',
    '-Wstrict-prototypes',
    '-Wimplicit-fallthrough=2',
    '-Wendif-labels',
    '-Wstrict-aliasing=2',
    '-Woverflow',

    '-Wno-missing-braces',
    '-Wno-missing-field-initializers',
    '-Wno-unused-parameters',
]), language: 'c')


glesv2 = dependency('glesv2')
xkbcommon = dependency('xkbcommon')
wayland_client = dependency('wayland-client')
wayland_server = dependency('wayland-server')
wayland_protocols = dependency('wayland-protocols')

# Try first to find wlroots as a subproject, then as a system dependency
wlroots_proj = subproject(
    'wlroots',
    default_options: ['examples=false'],
    required: false,
)
if wlroots_proj.found()
    wlroots = wlroots_proj.get_variable('wlroots')
else
    wlroots = dependency('wlroots')
endif

subdir('protocol')

ctwl_inc = include_directories('include')
ctwl_sources = files('src/main.c')
ctwl_deps = [glesv2,
        wlroots,
        xkbcommon,
        wayland_client,
        wayland_server,
        wayland_protocols,]

executable('c_tinywl',
    ctwl_sources + wl_protos_src,
    include_directories: [ctwl_inc],
    dependencies: ctwl_deps,
    install: true
)

