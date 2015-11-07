# alcazar-gen
A SAT-based generator for [Alcazar](http://www.theincrediblecompany.com/alcazar-1/) puzzles.

## Building
1. Install `cmake` and `boost`.
2. Run `make` to compile alcazar-gen
3. done

## Usage
Run `bin/alcazar-gen WIDTH HEIGHT` to generate an Alcazar puzzle with the dimensions WIDTH x HEIGHT.
Warning: generating puzzles with size > 5x5 may take a considerable amount of time.

    Usage: bin/alcazar-gen [OPTIONS]... [WIDTH HEIGHT]
    Allowed options:
      --help                Display this help message
      --seed arg            Set random seed
      --solve               Solve generated puzzle
      --template arg        Generate puzzle using the specified template file

## Template Files
You may either specify WIDTH and HEIGHT or a template file via the option "--template".

A template file for a WxH sized puzzle contains (2*H+1) lines with (2*W+1) of the following characters:
    +: an intersection of wall (just for decoration purposes) 
    .: a field (just for decoration purposes)
    | or -: a fixed closed wall position (the generated puzzle will have a wall in this position)
    /: a fixed open wall position (the generated puzzle will *not* have a wall in this position)
    ?: a possible wall position (the generated puzzle may have a wall in this position)

See the file(s) in the `templates` directory for examples.
