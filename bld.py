if __name__ == "__main__":
    import sys, subprocess # noqa : E401
    sys.argv[0:1] = [sys.executable, "-m", "SCons", "-f", __file__]
    sys.exit(subprocess.run(sys.argv).returncode)

################################################################################

import scons_msvc_env as msvc_env

env = msvc_env.MsvcEnvironment()
env.set_build_dir("src", "build")
env.Append(CPPPATH=[".", "pcre2_16", "romato/src"])
env.Append(CPPDEFINES=["UNICODE", "HAVE_CONFIG_H"])

env_pcre = env.Clone()
env_pcre.Append(CPPDEFINES=["_CRTIMP=", "_CRTIMP2_PURE=", "_VCRTIMP="])
env_pcre.modify_flags("CCFLAGS", ["/W3", "/w44244", "/w44267"], ["/W4"])
pcre_src = [
    "pcre2_16/pcre2_auto_possess.c",
    "pcre2_16/pcre2_chartables.c",
    "pcre2_16/pcre2_compile.c",
    "pcre2_16/pcre2_context.c",
    "pcre2_16/pcre2_find_bracket.c",
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
    env.ntdll_lib(),
    "kernel32.lib",
    "advapi32.lib",
    "user32.lib",
    "gdi32.lib",
    "shell32.lib",
    "shlwapi.lib",
    "comctl32.lib",
    "ole32.lib",
    ]
exe = env.Program("rgrep.exe", objs + res, LIBS=libs)

if env.sqaub_applicable():
    sexe = env.Squab(None, exe)
    env.Default(sexe)
else:
    env.Default(exe)
env.install_relative_to_parent_dir("bin", exe)
