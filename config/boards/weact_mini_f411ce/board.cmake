board_runner_args(dfu-util "--pid=0483:df11" "--alt=0" "--dfuse-address=0x08000000:leave")
board_runner_args(jlink "--device=STM32F411CE" "--speed=4000")
board_runner_args(pyocd "--target=stm32f411ce")

include(${ZEPHYR_BASE}/boards/common/dfu-util.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
