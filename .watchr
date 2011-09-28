watch("src/(.*)\.c") do |md|
  `mkdir -p ./dist`
  result = `gcc #{md[0]} -o ./dist/#{md[1]}.out 2>&1`
  system 'growlnotify', '-t', md[0], '-m', result.empty? ? 'Compiled successfully.' : result
end
