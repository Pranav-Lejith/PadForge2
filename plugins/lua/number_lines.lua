-- PadForge Lua plugin: Number Lines
-- Prefixes every line with its 1-based number.

function setup()
    padforge.add_menu_item("Number Lines", function()
        local text = padforge.get_text()
        local out = {}
        local n = 0
        for line in (text .. "\n"):gmatch("(.-)\n") do
            n = n + 1
            out[#out + 1] = string.format("%4d  %s", n, line)
        end
        padforge.set_text(table.concat(out, "\n"))
        padforge.message("Lines numbered")
    end)
end
