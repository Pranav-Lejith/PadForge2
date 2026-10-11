-- PadForge Lua plugin: Word Count
-- Adds a "Word Count" command that reports the line, word and character
-- counts of the active document in the status bar.

function setup()
    padforge.add_menu_item("Word Count", function()
        local text = padforge.get_text()
        local words = 0
        for _ in text:gmatch("%S+") do
            words = words + 1
        end
        padforge.message(string.format(
            "%d lines, %d words, %d characters",
            padforge.line_count(), words, #text))
    end)
end
