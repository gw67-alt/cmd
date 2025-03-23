def xor_decrypt(hex_string, key):
    # Convert hex string to bytes
    encrypted_bytes = bytes.fromhex(hex_string)
    
    # Repeat the key to match the length of the encrypted bytes
    key_bytes = key.encode()
    key_repeated = (key_bytes * (len(encrypted_bytes) // len(key_bytes) + 1))[:len(encrypted_bytes)]
    
    # Perform XOR decryption
    decrypted_bytes = bytes(a ^ b for a, b in zip(encrypted_bytes, key_repeated))
    
    return decrypted_bytes.decode(errors='ignore')


while True:
    # Example usage
    hex_string = input("Enter hex-encoded encrypted text: ")
    key = input("Enter the key: ")
    decrypted_text = xor_decrypt(hex_string, key)
    print("Decrypted text:", decrypted_text)
    print()