-- PadForge Lua plugin: Reverse Lines
-- Reverses the order of the lines in the active document.

function setup()
    padforge.add_menu_item("Reverse Lines", function()
        local text = padforge.get_text()
        local trailing = text:sub(-1) == "\n"
        local out = {}
        for line in (text .. "\n"):gmatch("(.-)\n") do
            out[#out + 1] = line
        end
        if trailing and out[#out] == "" then
            out[#out] = nil
        end
        local reversed = {}
        for i = #out, 1, -1 do
            reversed[#reversed + 1] = out[i]
        end
        local result = table.concat(reversed, "\n")
        if trailing then
            result = result .. "\n"
        end
        padforge.set_text(result)
        padforge.message("Lines reversed")
    end)
end
