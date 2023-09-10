if default_libc then
    includes("./lib"..default_libc.."/xmake.lua")
end 

if #cpp_apps >0 then 
    cpp_libs={'libcxx', 'libcxxabi'}
    for _, v in ipairs(cpp_libs) do
        includes("./"..v.."/xmake.lua")
    end
end


if #apps >0 then 
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
