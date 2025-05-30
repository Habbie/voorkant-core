project(
    'voorkant',
    ['cpp', 'c'],
    version: '0.1',
    default_options: ['warning_level=3', 'cpp_std=c++20'],
)

version_file = vcs_tag(input: 'src/version.cpp.in', output: 'version.cpp')

cpp = meson.get_compiler('cpp')

json_dep = dependency('nlohmann_json')

thread_dep = dependency('threads')

curl_dep = dependency(
    'libcurl',
    version: '>=8.5',
    allow_fallback: true,
    default_options: [
        'websockets=enabled',
        'tests=disabled',
        'bindlocal=disabled',
        'dict=disabled',
        'shuffle-dns=disabled',
        'doh=disabled',
        'file=disabled',
        'form-api=disabled',
        'ftp=disabled',
        'gopher=disabled',
        'imap=disabled',
        'ldap=disabled',
        'ldaps=disabled',
        'mime=disabled',
        'mqtt=disabled',
        'netrc=disabled',
        'pop3=disabled',
        'progress-meter=disabled',
        'psl=disabled',
        'sspi=disabled',
        'rtsp=disabled',
        'rtmp=disabled',
        'smb=disabled',
        'smtp=disabled',
        'socketpair=disabled',
        'telnet=disabled',
        'tftp=disabled',
        'tls-srp=disabled',
        'unixsockets=disabled',
        'openssl=disabled',
        'brotli=disabled',
        'http2=disabled',
        'zstd=disabled',
        'ssl=disabled',
        'ntlm=disabled',
        'kerberos-auth=disabled',
        'aws=disabled',
        'idn=disabled',
        'gss-api=disabled',
        'ssh=disabled',
        'tool=disabled',
        'default_library=static',
    ],
)

binh_prog = find_program('scripts/bin2header.py', required: true)
binh_generator = generator(
    binh_prog,
    output: '@PLAINNAME@.c',
    arguments: ['-v', '@EXTRA_ARGS@', '-i', '@INPUT@', '-o', '@OUTPUT@'],
)

# FIXME: the explicit 'B612_Regular_ttf' below would ideally be @PLAINNAME@.underscorify() above except substitution does not work there

font_b612_c = binh_generator.process(
    'src/fonts/polarsys-b612/fonts/ttf/B612-Regular.ttf',
    extra_args: 'B612_Regular_ttf',
)


font_mdi_c = binh_generator.process(
    'src/fonts/MaterialDesign-Webfont/fonts/materialdesignicons-webfont.ttf',
    extra_args: 'mdi_ttf',
)

mdimap_prog = find_program('scripts/mdimap.py', required: true)
mdimap_generator = generator(
    mdimap_prog,
    output: '@BASENAME@.cpp',
    arguments: ['@INPUT@', '@OUTPUT@'],
)
mdimap_cpp = mdimap_generator.process(
    'src/fonts/MaterialDesign-Webfont/scss/_variables.scss',
)

brew_prefix = '/opt/homebrew/include'
brew = find_program('brew', required: false)
if brew.found()
    cmd_brew_prefix = run_command('brew', '--prefix', check: false)
    if cmd_brew_prefix.returncode() == (0)
        brew_prefix = cmd_brew_prefix.stdout().strip()
    endif
endif

cmake = import('cmake')

# FIXME: we call this cmake_options but really it's mostly for LVGL, and we have separate cmake.subproject_options for quickjs. Need to rename.
cmake_options = cmake.subproject_options()
cmake_options.append_compile_args(
    'c',
    '-DLV_CONF_INCLUDE_SIMPLE',
    '-DLV_LVGL_H_INCLUDE_SIMPLE',
    '-DLV_CONF_PATH=' + meson.current_build_dir() + '/../src/lv_conf.h',
    '-I' + meson.current_build_dir() + '/../src',
    '-I' + meson.current_build_dir() + '/../subprojects/lvgl',
    '-I/' + brew_prefix + '/include',
)
cmake_options.append_compile_args(
    'cpp',
    '-DLV_CONF_INCLUDE_SIMPLE',
    '-DLV_LVGL_H_INCLUDE_SIMPLE',
    '-DLV_CONF_PATH=' + meson.current_build_dir() + '/../src/lv_conf.h',
    '-I' + meson.current_build_dir() + '/../src',
    '-I' + meson.current_build_dir() + '/../subprojects/lvgl',
    '-I/' + brew_prefix + '/include',
)
cmake_options.add_cmake_defines(
    {
        'LV_CONF_BUILD_DISABLE_EXAMPLES': true,
        'LV_CONF_BUILD_DISABLE_DEMOS': true,
    },
)

if cpp.links(
    '''int main(int argc, char **argv) { return 0; }''',
    args: '-latomic',
    name: 'libatomic',
)
    libatomic = declare_dependency(link_args: '-latomic')
else
    libatomic = dependency('', required: false)
endif

if get_option('front-lvgl').enabled()
    if get_option('lvgl-driver') == 'sdl'
        sdl2_dep = dependency('sdl2')
        cmake_options.append_compile_args('c', '-DVOORKANT_LVGL_SDL')
        cmake_options.append_compile_args('cpp', '-DVOORKANT_LVGL_SDL')
        lvgl_backend_define = '-DVOORKANT_LVGL_SDL'
    elif get_option('lvgl-driver') == 'fbdev'
        sdl2_dep = []
        cmake_options.append_compile_args('c', '-DVOORKANT_LVGL_FBDEV')
        cmake_options.append_compile_args('cpp', '-DVOORKANT_LVGL_FBDEV')
        lvgl_backend_define = '-DVOORKANT_LVGL_FBDEV'
    endif

    lvgl_proj = cmake.subproject('lvgl', options: cmake_options)
    lvgl_dep = lvgl_proj.dependency('lvgl')

    quickjs_options = cmake.subproject_options()
    quickjs_options.add_cmake_defines({'BUILD_QJS_LIBC': true})
    if libatomic.found()
        quickjs_options.append_compile_args('c', '-latomic')
    endif
    quickjs_proj = cmake.subproject('quickjs', options: quickjs_options)
    quickjs_dep = quickjs_proj.dependency('qjs')
endif

incdir = include_directories('./src/')

if get_option('front-ftxui').enabled()
    ftxui_screen_dep = dependency('ftxui-screen')
    ftxui_dom_dep = dependency('ftxui-dom')
    ftxui_component_dep = dependency('ftxui-component')

    executable(
        'voorkant-ftxui',
        [
            'src/main.cpp',
            'src/logger.cpp',
            'src/Backend.cpp',
            'src/HAEntity.cpp',
            'src/WSConn.cpp',
            'src/front-ftxui.cpp',
            'src/generated/domains.hpp',
            version_file,
        ],
        install: true,
        dependencies: [
            json_dep,
            thread_dep,
            curl_dep,
            ftxui_screen_dep,
            ftxui_dom_dep,
            ftxui_component_dep,
            libatomic,
        ],
    )
endif

executable(
    'voorkant-cli',
    [
        'src/main.cpp',
        'src/logger.cpp',
        'src/Backend.cpp',
        'src/Observer.hpp',
        'src/HAEntity.cpp',
        'src/WSConn.cpp',
        'src/front-cli.cpp',
        'src/generated/domains.hpp',
        version_file,
    ],
    install: true,
    dependencies: [
        json_dep,
        thread_dep,
        curl_dep,
        libatomic,
    ],
)

if get_option('front-lvgl').enabled()
    date_dep = dependency(
        'date',
        default_options: ['default_library=static', 'use_system_tzdb=true'],
    )

    executable(
        'voorkant-lvgl',
        [
            'src/main.cpp',
            'src/logger.cpp',
            'src/Backend.cpp',
            'src/HAEntity.cpp',
            'src/WSConn.cpp',
            'src/uicomponents/imgs/colorwheel24.c',
            'src/uicomponents/imgs/colortemp24.c',
            'src/uicomponents/imgs/brightness24.c',
            'src/uicomponents/imgs/white24.c',
            'src/uicomponents/uirgblight.cpp',
            'src/uicomponents/UILogBox.cpp',
            'src/uicomponents/uiapexcard.cpp',
            'src/uicomponents/UIComponents.cpp',
            'src/front-lvgl.cpp',
            'src/lv_conf.h',
            font_b612_c,
            font_mdi_c,
            'src/generated/domains.hpp',
            version_file,
            mdimap_cpp,
        ],
        install: true,
        dependencies: [
            json_dep,
            thread_dep,
            curl_dep,
            lvgl_dep,
            sdl2_dep,
            libatomic,
            date_dep,
            quickjs_dep,
        ],
        include_directories: incdir,
        cpp_args: [
            '-DLV_CONF_PATH=' + meson.current_build_dir() + '/../src/lv_conf.h',
            lvgl_backend_define,
        ],
    )
endif
