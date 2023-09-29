if default_libc then
    includes("./lib"..default_libc.."/xmake.lua")
end 

if has_config("cpp-apps") then 
    cpp_libs={'libcxx', 'libcxxabi'}
    for _, v in ipairs(cpp_libs) do
        includes("./"..v.."/xmake.lua")
    end
end


if has_config("apps") then 
    c_libs={
        'libjpeg', 'libgui', 'libetk', 'libzlib', 'libpng',
        'liblvgl', 'liblvgl-8.0.0', 'libuuid',
        'liblvqrcode', 'liblz4', 'libfreetype',
        'libwebp', 'libsdl2', 'libsdl2-ttf',
        'libsdl2-image', 'liblua',
        'libchez', 'libcmocka', 'libffmpeg',
        'libsdl', 'libsdl-ttf', 'libsdl-image',
        'libgaga',
        -- 'libiconv'
        'libncurses',
    }
    for _, v in ipairs(c_libs) do
        includes("./"..v.."/xmake.lua")
    end
end

if has_config("wayland") then 
    wayland_libs={'libwayland','libxml2'}
    for _, v in ipairs(wayland_libs) do
        includes("./"..v.."/xmake.lua")
    end
end