add_requires("benchmark 1.9.*", "fmt 11.0.*")

target("benchmark-ipc")
  set_kind("binary")
  add_deps("imp", "pmr", "ipc")
  add_packages("benchmark", "fmt")
  if is_os("windows") then
    add_ldflags("/subsystem:console")
  elseif is_os("linux") then
    add_syslinks("rt")
  end
  on_config(config_target_compilation)
  on_config(function (target)
    if target:has_tool("cxx", "cl") then
      target:add("defines", "_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING")
    else
      target:add("cxflags", "-Wno-missing-field-initializers"
                          , "-Wno-unused-variable"
                          , "-Wno-unused-function"
                          , "-Wno-unused-result")
    end
  end)
  add_includedirs("$(projectdir)/include"
                , "$(projectdir)/src"
                , "$(projectdir)/test")
  add_files("*.cpp")
