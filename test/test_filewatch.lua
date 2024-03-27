local lt = require "ltest"
local filewatch = require "bee.filewatch"
local fs = require "bee.filesystem"
local thread = require "bee.thread"
local supported = require "supported"

local test_fw = lt.test "filewatch"

local function create_file(filename, content)
    fs.remove(filename)
    local f <close> = assert(io.open(filename:string(), "wb"))
    if content ~= nil then
        f:write(content)
    end
end

local function test(f)
    local root = fs.absolute("./temp/"):lexically_normal()
    pcall(fs.remove_all, root)
    fs.create_directories(root)
    local fw = filewatch.create()
    fw:set_recursive(true)
    fw:set_follow_symlinks(true)
    fw:set_filter(function ()
        return true
    end)
    fw:add(root:string())
    f(fw, root)
    pcall(fs.remove_all, root)
end

function test_fw:test_1()
    test(function ()
    end)
end

function test_fw:test_2()
    test(function (fw, root)
        fs.create_directories(root / "dir")
        create_file(root / "file_1")
        fs.rename(root / "file_1", root / "file_2")
        fs.remove(root / "file_2")

        local function has(t, a)
            for _, v in ipairs(t) do
                if v == a then
                    return true
                end
            end
        end
        local list = {}
        local n = 100
        while true do
            local w, v = fw:select()
            if w then
                n = 100
                if not has(list, v) and root:string():match "^(.-)/?$" ~= v:match "^(.-)/?$" then
                    list[#list+1] = v
                end
            else
                n = n - 1
                if n < 0 then
                    break
                end
                thread.sleep(0.001)
            end
        end
        lt.assertEquals(list, {
            (root / "dir"):string(),
            (root / "file_1"):string(),
            (root / "file_2"):string(),
        })
    end)
end

-- test unexist symlink link to self
-- test directory symlink link to parent
function test_fw:test_symlink()
    if not supported "symlink" then
        return
    end

    local root = fs.absolute("./temp/"):lexically_normal()
    pcall(fs.remove_all, root)
    fs.create_directories(root)
    fs.create_symlink(root / "test1", root / "test1")
    fs.create_symlink(root, root / "child")

    local fw = filewatch.create()
    fw:set_recursive(true)
    fw:set_follow_symlinks(true)
    fw:add(root:string())
    pcall(fs.remove_all, root)
end
