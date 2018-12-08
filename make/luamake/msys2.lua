local fs = require 'bee.filesystem'

local function convertpath(s)
    return s:gsub("\\", "/"):gsub("([a-zA-z]):", "/%1")
end

local mt = {}

function mt:initialize(path)
    self.path = fs.path(path)
    self.bash = self.path / "usr" / "bin" / "bash.exe"
end

function mt:exec(cmd, cwd)
    cwd = cwd or fs.current_path():string()
    local command = ([[%s -lc "export PATH=/mingw64/bin:/usr/local/bin:/usr/bin:/bin:/opt/bin; cd %s; %s"]]):format(
        self.bash:string(),
        convertpath(cwd),
        convertpath(cmd)
    )
    local f = io.popen(command, 'r')
    for l in f:lines() do
        print(l)
    end
    f:close()
end

return mt
