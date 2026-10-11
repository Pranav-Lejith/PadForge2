-- PadForge Lua plugin: Trim Trailing Whitespace
-- Removes trailing spaces and tabs from every line of the active document.

local function trimRight(line)
    return (line:gsub("[ \t]+$", ""))
end

function setup()
    padforge.add_menu_item("Trim Trailing Whitespace", function()
        local text = padforge.get_text()
        local out = {}
        for line in (text .. "\n"):gmatch("(.-)\n") do
            out[#out + 1] = trimRight(line)
        end
        padforge.set_text(table.concat(out, "\n"))
        padforge.message("Trailing whitespace removed")
    end)
end
