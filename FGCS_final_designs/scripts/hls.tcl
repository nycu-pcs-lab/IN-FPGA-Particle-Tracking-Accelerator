# Invoke using: bash scripts/run_hls.sh Graph_M csim
# Vitis HLS 2023.2 does not forward trailing CLI tokens as Tcl argc/argv.
if {![info exists ::env(FGCS_DESIGN)]} { error "Use scripts/run_hls.sh Graph_M|Graph_L ?stage?" }
set family $::env(FGCS_DESIGN)
if {$family ni {Graph_M Graph_L}} { error "Unknown design" }
set stage csim
if {[info exists ::env(FGCS_STAGE)]} { set stage $::env(FGCS_STAGE) }
if {$stage ni {csim csynth cosim export all}} { error "Unknown stage" }
set root [file normalize [file join [file dirname [info script]] ..]]
set design [file join $root designs $family]
set build [file join $root .build $family]
set outputs [file join $root .build outputs $family]
file mkdir $outputs
# Fresh project per invocation; generated files stay under .build/.
open_project -reset $build
set_top su_top_model
add_files [file join $design src su_top_model.cpp] -cflags {-std=c++14}
add_files -tb [file join $design tb testbench.cpp] -cflags {-std=c++14}
open_solution -reset solution1 -flow_target vitis
set_part {xcvu9p-flga2104-2-e}
# Preserve the original HLS export target; this is not an achieved 200 MHz claim.
create_clock -period 5.000 -name default
set input [file join $root samples $family input.bin]
set weights [file join $root weights]
set actual [file join $outputs actual_raw.bin]
set tb_args [list $input $weights $actual]
if {$stage in {csim all}} { csim_design -clean -O -argv $tb_args }
if {$stage in {csynth cosim export all}} { csynth_design }
if {$stage in {cosim all}} {
    cosim_design -O -rtl verilog -tool xsim -trace_level none -argv $tb_args
}
if {$stage in {export all}} {
    export_design -format ip_catalog -rtl verilog -output [file join $outputs exported_ip.zip]
}
exit
