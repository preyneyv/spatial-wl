wl_protocols_dir = wayland_protocols.get_variable(pkgconfig: 'pkgdatadir')

wayland_scanner_dep = dependency('wayland-scanner', native: true)
wayland_scanner = find_program(
    wayland_scanner_dep.get_variable(pkgconfig: 'wayland_scanner'),
    native: true,
)

protocols = [
    wl_protocols_dir / 'stable/xdg-shell/xdg-shell.xml',
]

wl_protos_src = []

foreach xml : protocols
    wl_protos_src += custom_target(
        xml.underscorify() + '_c',
        input: xml,
        output: '@BASENAME@-protocol.c',
        command: [wayland_scanner, 'private-code', '@INPUT@', '@OUTPUT@']
    )
    wl_protos_src += custom_target(
        xml.underscorify() + '_server_h',
        input: xml,
        output: '@BASENAME@-protocol.h',
        command: [wayland_scanner, 'server-header', '@INPUT@', '@OUTPUT@']
    )
    wl_protos_src += custom_target(
        xml.underscorify() + '_client_h',
        input: xml,
        output: '@BASENAME@-client-protocol.h',
        command: [wayland_scanner, 'client-header', '@INPUT@', '@OUTPUT@']
    )
endforeach
