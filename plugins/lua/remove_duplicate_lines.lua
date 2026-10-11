-- PadForge Lua plugin: Remove Duplicate Lines
-- Keeps the first occurrence of each line (compared case-sensitively) and
-- removes later duplicates.

function setup()
    padforge.add_menu_item("Remove Duplicate Lines", function()
        local text = padforge.get_text()
        local trailing = text:sub(-1) == "\n"
        local seen = {}
        local out = {}
        for line in (text .. "\n"):gmatch("(.-)\n") do
            if not seen[line] then
                seen[line] = true
                out[#out + 1] = line
            end
        end
        if trailing and out[#out] == "" then
            out[#out] = nil
        end
        local result = table.concat(out, "\n")
        if trailing then
            result = result .. "\n"
        end
        padforge.set_text(result)
        padforge.message("Duplicate lines removed")
    end)
end
