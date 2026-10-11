-- PadForge Lua plugin: Sort Lines
-- Sorts the lines of the active document (case-insensitive ascending) and
-- writes the result back, preserving a trailing newline if one was present.

function setup()
    padforge.add_menu_item("Sort Lines", function()
        local text = padforge.get_text()
        local trailing = text:sub(-1) == "\n"
        local out = {}
        for line in (text .. "\n"):gmatch("(.-)\n") do
            out[#out + 1] = line
        end
        -- Drop the empty entry produced by a trailing newline.
        if trailing and out[#out] == "" then
            out[#out] = nil
        end
        table.sort(out, function(a, b)
            return a:lower() < b:lower()
        end)
        local result = table.concat(out, "\n")
        if trailing then
            result = result .. "\n"
        end
        padforge.set_text(result)
        padforge.message("Document sorted")
    end)
end
