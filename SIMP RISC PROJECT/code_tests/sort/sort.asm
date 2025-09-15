	.word  0x100 5      
	.word  0x101 8      
	.word  0x102 6      
	.word  0x103 -4      
	.word  0x104 4      
	.word  0x105 3      
	.word  0x106 -6      
	.word  0x107 -7      
	.word  0x108 12     
	.word  0x109 0     
	.word  0x10a 1     
	.word  0x10b 7     
	.word  0x10c -5     
	.word  0x10d 15     
	.word  0x10e 2     
	.word  0x10f 9      

	add $s0, $imm, $zero, 0x100     # $s0 = base address of array (0x100)
	add $a0, $imm, $zero, 0x10      # $a0 = array length (16)
L2:
    add $t0, $imm, $zero, 1			# $t0 = current index (starts at 1)
    add $a1, $imm, $zero, 0			# $a1 = last swap position (initialized to 0)
L1:
    lw $t2, $t0, $s0, 0				# $t2 = array[$t0] (current element)
    add $t0, $imm, $t0, -1			# $t0-- (move to previous index)
    lw $t1, $t0, $s0, 0				# $t1 = array[$t0] (previous element)
    bge $imm, $t2, $t1, finish		# if array[$t0] <= array[$t0+1], skip swap and jump to finish
    sw $t2, $t0, $s0, 0				# array[$t0] = $t2 (original array[$t0+1])
    add $t0, $imm, $t0, 1			# $t0++ (move back to original index)
    sw $t1, $t0, $s0, 0				# array[$t0] = $t1 (original array[$t0])
    add $a1, $t0, $zero, 0			# $a1 = $t0 (update last swap position)
    add $t0, $imm, $t0, -1			# $t0-- (prepare for next comparison)
finish:
    add $t0, $imm, $t0, 2			# $t0 += 2 (move to next pair)
    bgt $imm, $a0, $t0, L1			# if $t0 < $a0, jump to L1
    sub $a0, $a1, $zero, 0			# $a0 = $a1 (new length = last swap position)
    add $t0, $imm, $zero, 1			# $t0 = 1 (reset index)
    bgt $imm, $a0, $t0, L2			# if $a0 > 1, jump to L2
	halt $zero, $zero, $zero, 0     # End of program