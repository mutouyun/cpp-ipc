target("ipc")
  if has_config("build_shared_lib") then
    set_kind("shared")
    add_defines("LIBIMP_LIBRARY_SHARED_BUILDING__")
  else
    set_kind("static")
  end
  add_deps("imp", "pmr")
  if is_os("linux") then
    add_syslinks("pthread", "rt")
  elseif is_os("windows") then
    add_syslinks("Advapi32")
  end
  on_config(config_target_compilation)
  on_config(function (target)
    if (not target:has_tool("cxx", "cl")) and has_config("build_shared_lib") then
      target:add("linkgroups", "imp", "pmr", {whole = true})
    end
  end)
  add_includedirs("$(projectdir)/include", {public = true})
  add_includedirs("$(projectdir)/src")
  add_files("$(projectdir)/src/libipc/**.cpp")
