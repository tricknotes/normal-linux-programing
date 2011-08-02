watch("src/(.*)\.c") {|md| system "mkdir -p ./dist && gcc #{md[0]} -o ./dist/#{md[1]}.out" }
