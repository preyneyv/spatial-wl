wl_protocols_dir = wayland_protocols.get_variable(pkgconfig: 'pkgdatadir')

wayland_scanner_dep = dependency('wayland-scanner', native: true)
wayland_scanner = find_program(
    wayland_scanner_dep.get_variable(pkgconfig: 'wayland_scanner'),
    native: true,
)

protocols = [
    [wl_protocols_dir, 'stable/xdg-shell/xdg-shell.xml'],
]

wl_protocols_src = []
wl_protocols_headers = []

foreach p : protocols
    xml = join_paths(p)
    wl_protocols_src += custom_target(
        xml.underscorify() + '_c',
        input: xml,
        output: '@BASENAME@-protocol.c',
        command: [wayland_scanner, 'private-code', '@INPUT@', '@OUTPUT@']
    )
    wl_protocols_headers += custom_target(
        xml.underscorify() + '_server_h',
        input: xml,
        output: '@BASENAME@-protocol.h',
        command: [wayland_scanner, 'server-header', '@INPUT@', '@OUTPUT@']
    )
    wl_protocols_headers += custom_target(
        xml.underscorify() + '_client_h',
        input: xml,
        output: '@BASENAME@-client-protocol.h',
        command: [wayland_scanner, 'client-header', '@INPUT@', '@OUTPUT@']
    )
endforeach

lib_wl_protcools = static_library(
    'wl_protocols',
    wl_protocols_src + wl_protocols_headers,
    dependencies: wayland_client.partial_dependency(compile_args: true),
)

c_tinywl_protocols = declare_dependency(
    link_with: lib_wl_protcools,
    sources: wl_protocols_headers,
)
