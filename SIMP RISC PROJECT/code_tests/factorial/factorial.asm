	.word 0x100 18446744073709551614
	
	lw $a1, $imm, $zero, 0x100        # Load Input value from memory address 0x100
	add $t0, $imm, $zero, 1           # $t0 = 1 
	sll $sp, $t0, $imm, 11            # stack pointer $sp = 1 << 11 = 2048
	jal $ra, $imm, $zero, rec		  # Call rec(n), result stored in $v0
	sw $v0, $zero, $imm, 0x101        # Store rec result at memory address 0x101
	halt $zero, $zero, $zero, 0       # end program

rec:
    add $sp, $imm, $sp, -2			  # Allocate stack space for $ra and $a1
    sw $ra, $imm, $sp, 1              # Save return address
    sw $a1, $imm, $sp, 0              # Save current n value
    add $v0, $imm, $zero, 1           # return value = 1
    beq $imm, $zero, $a1, end		  # If n == 0, return 1
    add $a1, $imm, $a1, -1            # n = n - 1 
    jal $ra, $imm, $zero, rec		  # Call rec(n-1)
    lw $a1, $imm, $sp, 0              # Restore original n value from stack
    mul $v0, $a1, $v0, 0              # v0 = n * rec(n-1)

end:
    lw $a1, $imm, $sp, 0			  # Restore $a1
    lw $ra, $imm, $sp, 1			  # Restore return address
    add $sp, $imm, $sp, 2			  # Deallocate stack space
    bne $ra, $imm, $zero, 1			  # Return to caller
