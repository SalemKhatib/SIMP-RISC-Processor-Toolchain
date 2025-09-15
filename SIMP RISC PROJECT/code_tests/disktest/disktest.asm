	add $t1, $imm, $zero, irq        # $t1 = address of irq handler
	out $t1, $imm, $zero, 6          # Set pc of interrupt handler
	add $t1, $imm, $zero, 512        # $t1 = 512
	out $t1, $imm, $zero, 16         # Set disk buffer start
	add $t1, $imm, $zero, 0          # $t1 = 0 (starting sector)
	out $t1, $imm, $zero, 15         # Set current sector in disksector io register 
	add $t1, $imm, $zero, 1          # $t1 = 1
	out $t1, $imm, $zero, 1          # Enable irq1 irq1enable register

start:
    add $t1, $imm, $zero, 1      # $t1 = 1
    out $t1, $imm, $zero, 14     # Send read command set register to 1 (i/o register 14)

read:
    in $t1, $imm, $zero, 17      # Read disk status (i/o register 17)
    bne $imm, $zero, $t1, read	 # wait if busy 
    add $t0, $imm, $zero, 512    # $t0 = 512 start from sector 4

sum:
    add $t1, $imm, $t0, 128      # $t1 = $t0 + 128 
    lw $t2, $zero, $t0, 0        # $t2 = word from read buffer
    lw $t1, $zero, $t1, 0        # $t1 = word from write buffer
    add $t1, $t1, $t2, 0         # $t1 = sum of both words
    add $t2, $imm, $t0, 128      # $t2 = write buffer offset
    sw $t1, $zero, $t2, 0        # Store sum in write buffer
    add $t0, $imm, $t0, 1        # Increment buffer pointer
    add $t2, $imm, $zero, 768	 # $t2 = end of buffer 
    bgt $imm, $t2, $t0, sum      # Continue until end of buffer
    add $t0, $imm, $zero, 4      # $t0 = 4 
    in $t1, $imm, $zero, 15      # Read current sector 
    blt $imm, $t1, $t0, start	 # If not done, read next sector
    add $t1, $imm, $zero, 640	 # $t1 = write buffer address
    out $t1, $imm, $zero, 16     # Set disk buffer
    add $t1, $imm, $zero, 2      # $t1 = 2 write command
    out $t1, $imm, $zero, 14     #Send write command set register to 2(i/o register 14)

write:
    in $t1, $imm, $zero, 17      # Read disk status
    bne $imm, $zero, $t1, write  # wait if busy (diskstatus i/o register != 0)
    halt $zero, $zero, $zero, 0  # end program

irq:
    out $zero, $imm, $zero, 4    # irq1 interrupt 
    in $t1, $imm, $zero, 15      # Read current sector 
    add $t1, $imm, $t1, 1        # Increment sector number
    out $t1, $imm, $zero, 15     # Update sector 
	add $t1, $imm, $zero, 0		 # $t1 = 0 to stop the loop after reti
    reti $zero, $zero, $zero, 0  # Return from interrupt