	.word 0x100 0x5828    # Point A  
	.word 0x101 0x58D8    # Point B 
	.word 0x102 0xA8D8    # Point C  
	.word 0x103 0xA828    # Point D 
	
	add $t1, $imm, $zero, 0                # Initialize $t0 to 0
	lw $a1, $imm, $zero, 256               # Load first coordinate pair
	and $a0, $imm, $a1, 255                # Extract x1 coordinate
	srl $a1, $a1, $imm, 8                  # Extract y1 coordinate
	lw $a3, $imm, $zero, 258               # Load second coordinate pair
	and $a2, $imm, $a3, 255                # Extract x2 coordinate
	srl $a3, $a3, $imm, 8                  # Extract y2 coordinate

Loop:
    out $zero, $imm, $zero, 21			   # Set default pixel color to black
    out $t1, $imm, $zero, 20			   # Set current pixel position
    srl $t0, $t1, $imm, 8                  # Get current y position
    and $t2, $imm, $t1, 255                # Get current x position
    
    bgt $imm, $a1, $t0, end                # Skip if y < y1
    blt $imm, $a3, $t0, end                # Skip if y > y2
    bgt $imm, $a0, $t2, end                # Skip if x < x1
    blt $imm, $a2, $t2, end                # Skip if x > x2
    
    add $t0, $imm, $zero, 255              # Prepare white color
    out $t0, $imm, $zero, 21               # Set pixel to white

end:
    add $t0, $imm, $zero, 1                # Prepare pixel write command
    out $t0, $imm, $zero, 22               # Write the pixel
    add $t1, $imm, $t1, 1                  # Increment iterator
    add $t0, $imm, $zero, 65536            # Set maximum iteration value
    bgt $imm, $t0, $t1, Loop               # Repeat if not finished
    halt $zero, $zero, $zero, 0            # End program