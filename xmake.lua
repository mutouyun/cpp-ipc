set_project("cpp-ipc")
set_version("2.0.0", {build = "%Y%m%d%H%M"})

-- Build all of libipc's own tests.
option("build_tests") set_default(false)
-- Build all of libipc's own demos.
option("build_demos") set_default(false)
-- Build all of libipc's own benchmark tests.
option("build_benchmarks") set_default(false)
-- Build shared libraries (DLLs).
option("build_shared_lib") set_default(false)
-- Set to ON to build with static CRT on Windows (/MT).
option("use_static_crt") set_default(false)
-- Build with unit test coverage.
option("use_codecov") set_default(false)
option_end()

add_rules("mode.debug", "mode.release")
set_languages("cxx17")
if is_mode("debug") then
  if has_config("use_static_crt") then
    set_runtimes("MTd")
  else
    set_runtimes("MDd")
  end
else
  if has_config("use_static_crt") then
    set_runtimes("MT")
  else
    set_runtimes("MD")
  end
end

function config_target_compilation(target)
  if target:has_tool("cxx", "cl") then
    target:add("defines", "UNICODE", "_UNICODE")
    if is_mode("debug") then
      target:add("cxflags", "/Zi")
    end
  else
    target:add("cxflags", "-fPIC", "-Wno-attributes")
    if has_config("use_codecov") then
      target:add("cxflags", "--coverage")
      target:add("ldflags", "--coverage")
      target:add("syslinks", "gcov")
    end
    if is_mode("debug") then
      target:add("cxflags", "-rdynamic -fsanitize=address")
      target:add("ldflags", "-fsanitize=address")
    end
  end
end

includes("src")
if has_config("build_tests") then
  includes("test")
end
if has_config("build_demos") then
  includes("demo")
end
if has_config("build_benchmarks") then
  includes("benchmark")
end
