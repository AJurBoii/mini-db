describe 'database' do
    def run_script(commands)
        raw_output = nil
        IO.popen("./db", "r+") do |pipe|
            commands.each do |command|
                pipe.puts command
            end

            pipe.close_write

            raw_output = pipe.gets(nil)
        end
        raw_output.split("\n")
    end

    it 'inserts and retrieves a row' do
        result = run_script([
            "insert 1 aj amariuslesure@hotmail.com",
            "select",
            ".exit",
        ])
        expect(result).to match_array([
            "db > Executed.",
            "db > (1, aj, amariuslesure@hotmail.com)",
            "Executed.",
            "db > ",
        ])
    end
end