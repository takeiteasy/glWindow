# Generate glWindow.c - merge all source files

out = []
File.readlines("tools/template.c").each do |l|
    if l =~ /{{(src\/gl(\S+)\.c)}}/
        if !File.exist? $1
            out.append "#error #{$2} is not yet implemented!"
        else
            skip = true
            File.readlines($1).each do |ll|
                if skip
                    skip = false if ll =~ /^#include "gl(Window|Common)?\.[hc]"/
                else
                    out.append ll.rstrip
                end
            end
        end
    else
        out.append l.rstrip
    end
end
File.write "glWindow.c", out.join("\n")
