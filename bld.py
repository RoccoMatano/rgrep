if __name__ == '__main__':
    import sys, shutil, subprocess
    sys.argv[0:1] = [sys.executable, shutil.which("scons"), "-f", __file__]
    sys.exit(subprocess.run(sys.argv).returncode)

################################################################################

import msvc_env
cfg = msvc_env.BuildCfg()
env = msvc_env.MsvcEnvironment(cfg)
env.set_build_dir('src', 'build')
env.Append(CCFLAGS=['/DUNICODE', '/Isrc', '/Isrc/pcre2_16', '/Isrc/romato/src'])

env_pcre = env.Clone()
env_pcre.modify_flags(
    'CCFLAGS',
    ['/DHAVE_CONFIG_H', '/D_CRTIMP=', '/D_CRTIMP2_PURE=', '/D_VCRTIMP=', '/W3'],
    ['/W4']
    )
pcre_src = [
    "pcre2_16/pcre2_auto_possess.c",
    "pcre2_16/pcre2_chartables.c",
    "pcre2_16/pcre2_compile.c",
    "pcre2_16/pcre2_context.c",
    "pcre2_16/pcre2_find_bracket.c",
    "pcre2_16/pcre2_jit_compile.c",
    "pcre2_16/pcre2_match.c",
    "pcre2_16/pcre2_match_data.c",
    "pcre2_16/pcre2_newline.c",
    "pcre2_16/pcre2_ord2utf.c",
    "pcre2_16/pcre2_string_utils.c",
    "pcre2_16/pcre2_study.c",
    "pcre2_16/pcre2_substitute.c",
    "pcre2_16/pcre2_substring.c",
    "pcre2_16/pcre2_tables.c",
    "pcre2_16/pcre2_ucd.c",
    "pcre2_16/pcre2_valid_utf.c",
    "pcre2_16/pcre2_xclass.c",
    "pcre2_16/pcre2_adapt.c",
    ]
objs = env_pcre.Object(source=pcre_src)
objs += env.no_gl_object('romato/src/romato_no_ltcg.cpp')

env.use_pch()

env_mlib = env.Clone()
env_mlib.force_include_pch()
mlib_src = [
    "romato/src/base_dlg.cpp",
    "romato/src/font_size_dlg.cpp",
    "romato/src/list_ctrl.cpp",
    "romato/src/romato.cpp",
    "romato/src/resize_dlg_layout.cpp",
    "romato/src/yast.cpp",
    ]
objs += env_mlib.Object(source=mlib_src)

src = [
    "auto_complete_cb.cpp",
    "dir_iter.cpp",
    "rgrep.cpp",
    "rgrep_util.cpp",
    "rgrep_rx.cpp",
    "rgrep_dlg.cpp",
    "text_file.cpp",
    "search_thread.cpp",
    "settings_dlg.cpp",
    ]
objs += env.Object(source=src)
res = env.RES(source="res/rgrep.rc")

libs = [
    "kernel32.lib",
    "advapi32.lib",
    "user32.lib",
    "gdi32.lib",
    "shell32.lib",
    "shlwapi.lib",
    "comctl32.lib",
    "ole32.lib",
    ]
exe = env.Program('rgrep.exe', objs + res, LIBS=libs)

if env.cfg.arch == msvc_env.X64:
    # ignore command errors by prepending '-'
    env.Command(None, exe, Action('-squab $SOURCE', 'Squabbing $SOURCE'))
