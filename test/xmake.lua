add_requires("gtest 1.15.*", {configs = {main = true}})

target("test-ipc")
  set_kind("binary")
  add_deps("ipc")
  add_packages("gtest")
  add_links("gtest_main")
  if is_os("windows") then
    add_ldflags("/subsystem:console")
  end
  on_config(config_target_compilation)
  on_config(function (target)
    if target:has_tool("cxx", "cl") then
      target:add("cxflags", "/wd4723")
    else
      target:add("links", "gtest_main")
      target:add("cxflags", "-Wno-missing-field-initializers"
                          , "-Wno-unused-variable"
                          , "-Wno-unused-function")
    end
  end)
  add_includedirs("$(projectdir)/include"
                , "$(projectdir)/src"
                , "$(projectdir)/test")
  add_files("$(projectdir)/test/**.cpp")
