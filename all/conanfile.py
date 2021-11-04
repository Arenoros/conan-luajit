import os
import platform
from conans import ConanFile, tools, VisualStudioBuildEnvironment, AutoToolsBuildEnvironment


class LuajitConan(ConanFile):
    name = "luajit"
    license = "MIT"
    url = "https://github.com/conan-io/conan-center-index"
    homepage = "http://luajit.org"
    description = "LuaJIT is a Just-In-Time Compiler (JIT) for the Lua programming language."
    topics = ("conan", "lua", "jit")
    settings = "os", "arch", "compiler", "build_type"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "lua52compat":  [True, False],
        "nummode": [None, 1, 2],
        "with_gdbjit": [True, False],
        "with_apicheck": [True, False],
        "with_assert": [True, False],
        "with_ext": [True, False]
    }
    default_options = {
        "shared": False,
        "fPIC": True,
        "lua52compat":  True,
        "nummode": None,
        "with_gdbjit": False,
        "with_apicheck": False,
        "with_assert": False,
        "with_ext": True
    }
    _env_build = None
    exports_sources = ["ext/*"]
    @property
    def _source_subfolder(self):
        return "source_subfolder"

    def source(self):
        tools.get(**self.conan_data["sources"][self.version], strip_root=True, destination=self._source_subfolder)

    def configure(self):
        if self.options.shared:
            del self.options.fPIC
        del self.settings.compiler.libcxx
        del self.settings.compiler.cppstd

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def _configure_autotools(self):
        if not self._env_build:
            self._env_build = AutoToolsBuildEnvironment(self)
        return self._env_build

    def _get_defines(self):
        defines = []
        if self.options.lua52compat or self.version == "2.1.0-b3-openresty":
            #2.1.0-b3-openresty : lj_recdef.h:254:1: error: ‘recff_rawlen’ undeclared here (not in a function); did you mean ‘recff_rawset’?
            defines.append('LUAJIT_ENABLE_LUA52COMPAT')

        if self.options.nummode:
            defines.append(f"LUAJIT_NUMMODE={self.options.nummode}")
        if self.options.with_gdbjit:
            defines.append('LUAJIT_USE_GDBJIT')
        if self.options.with_apicheck:
            defines.append('LUA_USE_APICHECK')
        if self.options.with_assert:
            defines.append('LUA_USE_ASSERT')

    def _msvc_build(self):
        msvcbuild = os.path.join(self._source_subfolder, 'src', 'msvcbuild.bat')
        defs = self._get_defines()
        tools.replace_in_file(msvcbuild,
                        'NAME=lua51',
                        'NAME=luajit')
        tools.replace_in_file(msvcbuild,
                        'luajit.exe',
                        'luajitc.exe')
                        
        err_text = '@echo *** Build FAILED -- Please check the error messages ***'
        tools.replace_in_file(msvcbuild,
                                    err_text,
                                    err_text + ' & exit 1')
        if defs:
            tools.replace_in_file(msvcbuild,
                                    '@set LJCOMPILE=cl',
                                    '@set LJCOMPILE=cl /D' + defs.join(' /D'))
        if self.options.with_ext:
            tools.replace_in_file(msvcbuild,
                                    '@del *.obj',
                                    '@del *.obj ext/*.obj')
            tools.replace_in_file(msvcbuild,
                                    'lj_*.c',
                                    'lj_*.c ext/*.c')
            tools.replace_in_file(msvcbuild,
                                    'lj_*.obj',
                                    'lj_*.obj ext_*.obj')
                                    
        if self.settings.build_type == "Debug":
            tools.replace_in_file(msvcbuild,
                                    '@if "%1" neq "debug" goto :NODEBUG',
                                    '@if "%BUILDTYPE%" neq "debug" goto :NODEBUG')
            tools.replace_in_file(msvcbuild,
                                    '@set BUILDTYPE=release',
                                    '@set BUILDTYPE=debug')

        with tools.chdir(os.path.join(self._source_subfolder, 'src')):
                env_build = VisualStudioBuildEnvironment(self)
                with tools.environment_append(env_build.vars), tools.vcvars(self):
                    variant = '' if self.options.shared else 'static'
                    self.run("msvcbuild.bat %s" % variant)

    def _makefile_build(self):
        makefile = os.path.join(self._source_subfolder, 'src', 'Makefile')
        buildmode = 'shared' if self.options.shared else 'static'
        defs = self._get_defines()
        if defs:
            tools.replace_in_file(makefile,
                                    'XCFLAGS=',
                                    'XCFLAGS= -D' + defs.join(' -D'))
                                
        tools.replace_in_file(makefile,
                                'BUILDMODE= mixed',
                                'BUILDMODE= %s' % buildmode)
        tools.replace_in_file(makefile,
                                'TARGET_DYLIBPATH= $(TARGET_LIBPATH)/$(TARGET_DYLIBNAME)',
                                'TARGET_DYLIBPATH= $(TARGET_DYLIBNAME)')
        # adjust mixed mode defaults to build either .so or .a, but not both
        if not self.options.shared:
            tools.replace_in_file(makefile,
                                    'TARGET_T= $(LUAJIT_T) $(LUAJIT_SO)',
                                    'TARGET_T= $(LUAJIT_T) $(LUAJIT_A)')
            tools.replace_in_file(makefile,
                                    'TARGET_DEP= $(LIB_VMDEF) $(LUAJIT_SO)',
                                    'TARGET_DEP= $(LIB_VMDEF) $(LUAJIT_A)')
        else:
            tools.replace_in_file(makefile,
                                    'TARGET_O= $(LUAJIT_A)',
                                    'TARGET_O= $(LUAJIT_SO)')
        if self.options.with_ext:
            tools.replace_in_file(makefile,
                                    'ALL_RM=',
                                    'ALL_RM= ext/*.o ')
            tools.replace_in_file(makefile,
                                    'LJCORE_O= lj_assert.o',
                                    'LJEXT_C= $(wildcard ext/*.c)\n' \
                                    'LJEXT_O= $(LJEXT_C:.c=.o)\n' \
                                    'LJCORE_O= $(LJEXT_O) lj_assert.o')

        if self.settings.build_type in ["Debug", 'RelWithDebInfo']:
            tools.replace_in_file(makefile,
                                    '#CCDEBUG=',
                                    '#CCDEBUG= -g')
            
        env = dict()
        if self.settings.os == "Macos":
            # Per https://luajit.org/install.html: If MACOSX_DEPLOYMENT_TARGET
            # is not set then it's forced to 10.4, which breaks compile on Mojave.
            version = self.settings.get_safe("os.version")
            if not version and platform.system() == "Darwin":
                major, minor, _ = platform.mac_ver()[0].split(".")
                version = "%s.%s" % (major, minor)
            env["MACOSX_DEPLOYMENT_TARGET"] = version
        with tools.chdir(self._source_subfolder), tools.environment_append(env):
            env_build = self._configure_autotools()
            env_build.make(args=["PREFIX=%s" % self.package_folder])

    def build(self):
        tools.rename("ext", os.path.join(self._source_subfolder, 'src', 'ext'))
        if self.settings.compiler == 'Visual Studio':
            self._msvc_build()
        else:
            self._makefile_build()

    def package(self):
        self.copy("COPYRIGHT", dst="licenses", src=self._source_subfolder)
        ljs = os.path.join(self._source_subfolder, "src")
        inc = os.path.join(self.package_folder, "include", "luajit")
        if self.settings.compiler == 'Visual Studio':
            self.copy("lua.h", dst=inc, src=ljs)
            self.copy("lualib.h", dst=inc, src=ljs)
            self.copy("lauxlib.h", dst=inc, src=ljs)
            self.copy("luaconf.h", dst=inc, src=ljs)
            self.copy("lua.hpp", dst=inc, src=ljs)
            self.copy("luajit.h", dst=inc, src=ljs)
            self.copy("luajit.lib", dst="lib", src=ljs)
            self.copy("luajit.dll", dst="bin", src=ljs)
            self.copy("luajit.pdb", dst="bin", src=ljs)
        else:
            with tools.chdir(self._source_subfolder):
                env_build = self._configure_autotools()
                env_build.install(args=["PREFIX=%s" % self.package_folder])
            tools.rmdir(os.path.join(self.package_folder, "lib", "pkgconfig"))
            tools.rmdir(os.path.join(self.package_folder, "share"))

        if self.options.with_ext:
            self.copy("ext/*.h", dst=inc, src=ljs)
    def package_info(self):
        self.cpp_info.libs = ["luajit" if self.settings.compiler == "Visual Studio" else "luajit-5.1"]
        self.cpp_info.includedirs = ['include'] # [os.path.join(self.package_folder, "include", "luajit-2.0")]
        if self.settings.os == "Linux":
            self.cpp_info.system_libs.extend(["m", "dl"])
