#include"loader.h"

Elf32_Ehdr *ehdr;
Elf32_Phdr *phdr;
int file_desc;
void loader_cleanup() {
  free(phdr);
  free(ehdr);
  close(file_desc);
 
}

// 1. Load entire binary content into the memory from the ELF file.
// 2. Iterate through the PHDR table and find the section of PT_LOAD 
//    type that contains the address of the entrypoint method in fib.c
// 3. Allocate memory of the size "p_memsz" using mmap function 
//    and then copy the segment content
// 4. Navigate to the entrypoint address into the segment loaded in the memory in above step
// 5. Typecast the address to that of function pointer matching "_start" method in fib.c.
// 6. Call the "_start" method and print the value returned from the "_start"

void load_and_run_elf(char **exe) {

file_desc = open(exe[1], O_RDONLY);
    if (file_desc < 0) {
        perror("Error opening file");
        exit(1);
    }

    // Initialize entry address for the main function
    unsigned int entry_address = 0;

    // Allocate memory for the ELF header
    ehdr = malloc(sizeof(Elf32_Ehdr));
    if (ehdr == NULL) {
        perror("Error allocating memory for ELF header");
        close(file_desc);
        exit(1);
    }

    // Read the ELF header from the file
    int readelf = read(file_desc, ehdr, sizeof(Elf32_Ehdr));
    if (readelf != sizeof(Elf32_Ehdr)) {
        perror("Error reading ELF header");
        free(ehdr);
        close(file_desc);
        exit(1);
    }

    // Get the number of program headers and size of each
    int phdr_count = ehdr->e_phnum;
    int phdr_size = ehdr->e_phentsize;

    // Allocate memory for the program header table
    phdr = malloc(phdr_size * phdr_count);
    if (phdr == NULL) {
        perror("Error allocating memory for program headers");
        free(ehdr);
        close(file_desc);
        exit(1);
    }

    // Seek to the start of the program header table
    off_t phdr_offset = ehdr->e_phoff;
    int seek_to_phdr = lseek(file_desc, phdr_offset, SEEK_SET);
    if (seek_to_phdr == (off_t)-1) {
        perror("Error seeking to program headers");
        free(phdr);
        free(ehdr);
        close(file_desc);
        exit(1);
    }

    // Read the program headers
    int read_phdr_struct = read(file_desc, phdr, phdr_size * phdr_count);
    if (read_phdr_struct != phdr_size * phdr_count) {
        perror("Error reading program headers");
        free(phdr);
        free(ehdr);
        close(file_desc);
        exit(1);
    }

    // Iterate through the program headers to load segments
    int i = 0;
    while( i < phdr_count) {
        if (phdr[i].p_type == PT_LOAD) {
            // Map the segment into memory with appropriate permissions
            void *mapped_mem = mmap(
                (void *)phdr[i].p_vaddr,
                phdr[i].p_memsz,
                PROT_READ | PROT_WRITE | PROT_EXEC,
                MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED,
                -1, 0
            );
            if (mapped_mem == MAP_FAILED) {
                perror("Error mapping segment into memory");
                free(phdr);
                free(ehdr);
                close(file_desc);
                exit(1);
            }

            // Move file pointer to segment offset and read segment into memory
            int seek_to_data = lseek(file_desc, phdr[i].p_offset, SEEK_SET);
            if (seek_to_data == (off_t)-1) {
                perror("Error seeking to segment data");
                free(phdr);
                free(ehdr);
                close(file_desc);
                exit(1);
            }

            int read_data = read(file_desc, mapped_mem, phdr[i].p_filesz);
            if (read_data != phdr[i].p_filesz) {
                perror("Error reading segment data");
                free(phdr);
                free(ehdr);
                close(file_desc);
                exit(1);
            }
        }
         i++;
    }

    // Retrieve the entry point address from the ELF header
    entry_address = ehdr->e_entry;

    // Cast the entry address to a function pointer and execute
    int (*_start)(void) = (int (*)(void))entry_address;
    int result = _start();

    // Output the return value from the _start function
    printf("Function _start returned value = %d\n", result);


    
}
int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <ELF Executable>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
   
    load_and_run_elf(argv);
    loader_cleanup();

    return 0;
}
