//
//  main.cpp
//  FileSystem
//
//  Created by Eren Sanlıer on 27.05.2020.
//  Copyright © 2020 Eren Sanlıer. All rights reserved.
//

#include <iostream>
#include <fstream>
#include <string>
#include <stdio.h>
#include <cstdlib>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <chrono>

int BLOCK_SIZE;
int DIRECTORY_SIZE = 32768;
int remainingSpace = DIRECTORY_SIZE;
int directoryContent[32768];

unsigned int lastId = 0;

int createError = 0;
int extendError = 0;

typedef struct DirectoryEntry{
    int start_index;
    int file_size;
}DirectoryEntry;


std::unordered_map<int, DirectoryEntry> directoryTable;
int FAT[32768];



//Parent class for allocators
class Allocator{
    
public:
    
    void fillRandom(int start_index, int file_size){
        for(int i = start_index; i < file_size + start_index; i++){
            int temp = rand() % 10 + 1;
            directoryContent[i] = temp;
        }
    }
    virtual int create_file(int file_id, int file_length) = 0;
    virtual int access(int file_id, int byte_offset) = 0;
    virtual int extend(int file_id, int extension) = 0;
    virtual int shrink(int file_id, int shrinking) = 0;
};

class ContigiousAllocator : public Allocator {
    
    //This method is to find the first available block's start address
    int firstFitFinder(int file_length){
        
        if(remainingSpace < file_length) return -1;
        else if(directoryTable.size() == 0) return 0;
        
        //Lets find first entry
        DirectoryEntry *entry = &directoryTable.begin()->second;
        std::unordered_map<int, DirectoryEntry>::iterator it;
        for (it = directoryTable.begin(); it != directoryTable.end(); it++){
            if(it->second.start_index < entry->start_index){
                entry = &it->second;
            }
        }
        
        int last_fileend = 0;
        
        while(1){
            int left_gap = entry->start_index - last_fileend;
            if(left_gap >= file_length) return last_fileend;
            DirectoryEntry *next = findNext(entry);
            if(entry == next){
                if(DIRECTORY_SIZE - (entry->file_size + entry->start_index) >= file_length) return entry->file_size + entry->start_index;
                else return -1;
            }else{
                last_fileend = entry->start_index + entry->file_size;
                entry = next;
            }
        }
        
    }
    
    //Returns successor file of an entry. Returns entry itself if it is the last one.
    DirectoryEntry* findNext(DirectoryEntry* entry){
        int entry_start = entry->start_index;
        int entry_filesize = entry->file_size;
        int entry_fileend = entry_start + entry_filesize - 1;
        
        std::unordered_map<int, DirectoryEntry>::iterator it;
        DirectoryEntry *next_entry = entry;
        int latest_index = -1;
        for (it = directoryTable.begin(); it != directoryTable.end(); it++){
            if(it->second.start_index > entry_fileend && (it->second.start_index <= latest_index || latest_index == -1)){
                next_entry = &it->second;
                latest_index = next_entry->start_index;
            }
        }
        return next_entry;
    }
    
    void shiftFile(DirectoryEntry *entry, int shift_amt){
        int start = entry->start_index;
        int file_size = entry->file_size;
        for(int i = start; i < start + file_size; i++){
            directoryContent[i - shift_amt] = directoryContent[i];
            directoryContent[i] = 0;
        }
        entry->start_index -= shift_amt;
    }
    
    void defragmentation(){
        
        if(directoryTable.size() == 0) return;
        
        //Lets find first entry
        DirectoryEntry *entry = &directoryTable.begin()->second;
        std::unordered_map<int, DirectoryEntry>::iterator it;
        for (it = directoryTable.begin(); it != directoryTable.end(); it++){
            if(it->second.start_index < entry->start_index){
                entry = &it->second;
            }
        }
        
        int last_fileend = 0;
        DirectoryEntry *next;
        
        while(1){
            int shift_left = entry->start_index - last_fileend;
            if(shift_left > 0) shiftFile(entry, shift_left);
            last_fileend = entry->start_index + entry->file_size;
            next = findNext(entry);
            if(next == entry) break; //Indicates that this is the last file
            entry = next;
        }
    }
    
public:

    virtual int create_file(int file_id, int file_length){
        
        if(remainingSpace < file_length) return -1;
        
        int start_index = firstFitFinder(file_length);
        //if hole found, go on and allocate
        if(start_index >= 0){
             DirectoryEntry entry;
             entry.file_size = file_length;
             entry.start_index = start_index;
             directoryTable.insert(std::pair<int, DirectoryEntry>(file_id, entry));
            lastId++;
             fillRandom(entry.start_index, entry.file_size);
            remainingSpace -= file_length;
             return start_index;
         }else{
             defragmentation();
             start_index = firstFitFinder(file_length);
             if(start_index >= 0){
                DirectoryEntry entry;
                entry.file_size = file_length;
                entry.start_index = start_index;
                directoryTable.insert(std::pair<int, DirectoryEntry>(file_id, entry));
                 lastId++;
                fillRandom(entry.start_index, entry.file_size);
                 remainingSpace -= file_length;
                 return start_index;
             }else{
                 return -1;
             }
         }
    }
    
    virtual int access(int file_id, int byte_offset){
        std::unordered_map<int, DirectoryEntry>::iterator it = directoryTable.find(file_id);
        if(it == directoryTable.end()) return -1;
        DirectoryEntry *entry = &it->second;
        if(byte_offset/BLOCK_SIZE <= entry->file_size){
            return entry->start_index + byte_offset/BLOCK_SIZE;
        }else{
            return -1;
        }
    }
    
    virtual int extend(int file_id, int extension){
        
        //No space left, return error
        if(remainingSpace < extension) return -1;
        
        //If file doesn't exist
        std::unordered_map<int, DirectoryEntry>::iterator it = directoryTable.find(file_id);
        if(it == directoryTable.end()) return -1;
        
        DirectoryEntry *entry = &it->second;
        DirectoryEntry *nextEntry = findNext(entry);
        if(entry == nextEntry){
            //if entry is the last one
            if(entry->start_index + entry->file_size + extension <= DIRECTORY_SIZE){
                //Enough space next to file
                fillRandom(entry->start_index + entry->file_size, extension);
                entry->file_size += extension;
                remainingSpace -= extension;
            }else{
                defragmentation();
                //Enough space next to file
                fillRandom(entry->start_index + entry->file_size, extension);
                entry->file_size += extension;
                remainingSpace -= extension;
            }
            return 1;
        }
        //if entry is not the last one
        //Enough space next to file
        if(entry->start_index + entry->file_size + extension <= nextEntry->start_index){
            fillRandom(entry->start_index + entry->file_size, extension);
            entry->file_size += extension;
            remainingSpace -= extension;
            return 1;
        }
        //Should do something to move the file to the end or make a gap. First you should try to find a space in directory and move
        int start_index = firstFitFinder(entry->file_size + extension);
        if(start_index >= 0){
            for(int i = entry->start_index; i < entry->start_index + entry->file_size; i++){
                directoryContent[start_index + i - entry->start_index] = directoryContent[i];
                directoryContent[i] = 0;
            }
            entry->start_index = start_index;
            fillRandom(entry->start_index + entry->file_size, extension);
            entry->file_size += extension;
            remainingSpace -= extension;
            return 1;
        }
        //Couldn't find space, lets defragment once and try again
        defragmentation();
        start_index = firstFitFinder(entry->file_size + extension);
        if(start_index >= 0){
            for(int i = entry->start_index; i < entry->start_index + entry->file_size; i++){
                directoryContent[start_index + i - entry->start_index] = directoryContent[i];
                directoryContent[i] = 0;
            }
            entry->start_index = start_index;
            fillRandom(entry->start_index + entry->file_size, extension);
            entry->file_size += extension;
            remainingSpace -= extension;
            return 1;
        }
        //After this point only option left is to move file to end, defrag again and extend. If not doable we stop since file has to stay contigious.
        start_index = firstFitFinder(entry->file_size);
        if(start_index >= 0){
            for(int i = entry->start_index; i < entry->start_index + entry->file_size; i++){
                directoryContent[start_index + i - entry->start_index] = directoryContent[i];
                directoryContent[i] = 0;
            }
            entry->start_index = start_index;
            defragmentation();
            //At this point our file is in the end and we have defragmented. We can safely extend.
            fillRandom(entry->start_index + entry->file_size, extension);
            entry->file_size += extension;
            remainingSpace -= extension;
            return 1;
        }
        return -1;
    }
    
    virtual int shrink(int file_id, int shrinking){
        //Check if file exists
        std::unordered_map<int, DirectoryEntry>::iterator it = directoryTable.find(file_id);
        if(it == directoryTable.end()) return -1;
        
        DirectoryEntry *entry = &it->second;
        if(shrinking < entry->file_size){ // shrink by the given value
            entry->file_size -= shrinking;
            int file_end = entry->start_index + entry->file_size;
            for(int i = file_end; i < file_end + shrinking; i++){
                directoryContent[i] = 0;
            }
            remainingSpace += shrinking;
            return 1;
        }else{
            return -1;
        }
    }
};

class LinkedAllocator : public Allocator {
    
    int findAvailableBlock(){
        if(remainingSpace == 0) return -1;
        int i = 0;
        while(directoryContent[i] != 0){
            i++;
        }
        return i;
    }
    int findFileEnd(DirectoryEntry* entry){
        int index = entry->start_index;
        int file_size = entry->file_size;
        for(int i = 1; i < file_size; i++){
            index = FAT[index];
        }
        return index;
    }
    
    
public:
    virtual int create_file(int file_id, int file_length){
        if(remainingSpace < file_length) return -1;
        int last_block = findAvailableBlock();
        DirectoryEntry entry;
        entry.start_index = last_block;
        entry.file_size = file_length;
        directoryTable.insert(std::pair<int, DirectoryEntry>(file_id, entry));
        lastId++;
        
        fillRandom(last_block, 1);
        for(int i = 1; i < file_length; i++){
            int block = findAvailableBlock();
            FAT[last_block] = block;
            fillRandom(block, 1);
            last_block = block;
        }
        remainingSpace -= file_length;
        return 1;
    }
    
    virtual int access(int file_id, int byte_offset){
        int block_number = byte_offset / BLOCK_SIZE; // Although we use 4 bytes of block for FAT, the user sees reduced size.
        std::unordered_map<int, DirectoryEntry>::iterator it = directoryTable.find(file_id);
        if(it == directoryTable.end()) return -1;
        DirectoryEntry *entry = &it->second;
        int index = entry->start_index;
        if(block_number <= entry->file_size){
            for(int i = 1; i < block_number; i++){
                index = FAT[index];
            }
            return index;
        }else return -1;
    }
    
    virtual int extend(int file_id, int extension){
        if(remainingSpace < extension) return -1;
        std::unordered_map<int, DirectoryEntry>::iterator it = directoryTable.find(file_id);
        if(it == directoryTable.end()) return -1;
        
        DirectoryEntry *entry = &it->second;
        int file_end = findFileEnd(entry);
        
        for(int i = 0; i < extension; i++){
            int block = findAvailableBlock();
            FAT[file_end] = block;
            fillRandom(block, 1);
            file_end = block;
        }
        entry->file_size += extension;
        remainingSpace -= extension;
        return 1;
    }
    
    virtual int shrink(int file_id, int shrinking){
        std::unordered_map<int, DirectoryEntry>::iterator it = directoryTable.find(file_id);
        if(it == directoryTable.end()) return -1;
        DirectoryEntry *entry = &it->second;
        int file_size = entry->file_size;
        if(shrinking < entry->file_size){ // shrink by the given value
            int index = entry->start_index;
            for(int i = 1; i < file_size - shrinking; i++){
                index = FAT[index];
            }
            for(int i = 0; i < shrinking; i++){
                int buffer = FAT[index];
                directoryContent[buffer] = 0;
                FAT[index] = -1;
                index = buffer;
            }
            entry->file_size -= shrinking;
            remainingSpace += shrinking;
            return 1;
        }else{
            return -1;
        }
    }
};


void parseAndExec(Allocator *a, std::string command){
    std::string delimiter = ":";
    size_t pos = command.find(delimiter);
    std::string operation = command.substr(0, pos);
    command.erase(0, pos + 1);
    
    if(!operation.compare("c")){
        pos = command.find(delimiter);
        int arg = std::stoi(command.substr(0, pos));
        double nBlocks = std::ceil((double) arg / BLOCK_SIZE);
        if(a->create_file(lastId, nBlocks) == -1){
            //std::cout << " Create Error" << std::endl;
            createError++;
        }
        //else std::cout << " Create successful" << std::endl;
        //std::cout << " Remaining space: " << remainingSpace;
    }else{
        pos = command.find(delimiter);
        int arg1 = std::stoi(command.substr(0, pos));
        command.erase(0, pos + 1);
        pos = command.find(delimiter);
        int arg2 = std::stoi(command.substr(0, pos));
        if(!operation.compare("sh")){
             //if(a->shrink(arg1, arg2) == -1) std::cout << " Shrink Error" << std::endl;
             //else std::cout << " Shrink successful" << std::endl;
        }
        else if(!operation.compare("a")){
            //if(a->access(arg1, arg2) == -1) std::cout << " Access Error" << std::endl;
            //else std::cout << " Accessed the address" << std::endl;
        }
        else if(!operation.compare("e")){
            if(a->extend(arg1, arg2) == -1){
                //std::cout << " Extend Error" << std::endl;
                extendError++;
            }
            //else std::cout << " Extend successful" << std::endl;
        }
        //std::cout << " Remaining space: " << remainingSpace;
        return;
    }
    
    
}

int main(int argc, const char * argv[]) {
    
    //Init allocator
    Allocator *a;
    
    //Take input file
    std::string file_name = argv[1];
    int digits = (int) file_name.substr(6,file_name.size() - 1).find('_');
    //Getting block size by string manupilation
    BLOCK_SIZE = std::stoi(file_name.substr(6, 6 + digits));
    
    std::string method_input = argv[2];
    if(!method_input.compare("-c")){
        std::cout << "Contigious" << std::endl;
        a = new ContigiousAllocator;
    }else if(!method_input.compare("-l")){
        remainingSpace -= DIRECTORY_SIZE * 4 / BLOCK_SIZE;
        BLOCK_SIZE -= 4; //The space required for pointers
        for(int i = 0; i < sizeof(FAT) / sizeof(int); i++){
            FAT[i] = -1;
        }
        std::cout << "Linked" << std::endl;
        a = new LinkedAllocator;
    }else return -1;
    
    
    std::string line;
    std::ifstream fileOp(file_name);
    
    // Record start time
    auto start = std::chrono::high_resolution_clock::now();

    while(getline(fileOp, line)) {
        // Output the text from the file
        parseAndExec(a, line);
    }

    // Record end time
    auto finish = std::chrono::high_resolution_clock::now();
    
    std::chrono::duration<double> elapsed = finish - start;
    
    std::cout << "\nElapsed time: " << elapsed.count() << " s\nCreate Errors: " << createError << " Extend Errors: " << extendError;
    
    fileOp.close();
    
    /*
    std::cout << "-----------------------Write Test---------------------" << std::endl;
    for(int i = 0; i < 8; i++){
        a->create_file(i, 4);
        std::cout << "Directory Content ->" << std::endl;
        for(int i = 0; i < 50; i++){
            std::cout << directoryContent[i] << " ";
        }
        std::cout << std::endl;
        std::unordered_map<int, DirectoryEntry>::iterator it;
        std::cout << "Directory Table ->" << std::endl;
        for (it = directoryTable.begin(); it != directoryTable.end(); it++){
            std::cout << it->first << " - " << it->second.start_index << " : " << it->second.file_size << " | " ;
        }
        std::cout << std::endl << "FAT ->" << std::endl;
        for(int i = 0; i < 50; i++) std::cout << FAT[i] << " ";
        std::cout << std::endl;
    }
    std::cout << "------------------------------------------------------" << std::endl;
    std::cout << "-----------------------Extend Test---------------------" << std::endl;
    for(int i = 0; i < 8; i++){
        a->extend(i, 2);
        std::cout << "Directory Content ->" << std::endl;
        for(int i = 0; i < 50; i++){
            std::cout << directoryContent[i] << " ";
        }
        std::cout << std::endl;
        std::unordered_map<int, DirectoryEntry>::iterator it;
        std::cout << "Directory Table ->" << std::endl;
        for (it = directoryTable.begin(); it != directoryTable.end(); it++){
            std::cout << it->first << " - " << it->second.start_index << " : " << it->second.file_size << " | " ;
        }
        std::cout << std::endl << "FAT ->" << std::endl;
        for(int i = 0; i < 50; i++) std::cout << FAT[i] << " ";
        std::cout << std::endl;
    }
    
    std::cout << "------------------------------------------------------" << std::endl;
    
    std::cout << "-----------------------Shrink Test---------------------" << std::endl;
    for(int i = 0; i < 8; i++){
        a->shrink(i, 3);
        std::cout << "Directory Content ->" << std::endl;
        for(int i = 0; i < 50; i++){
            std::cout << directoryContent[i] << " ";
        }
        std::cout << std::endl;
        std::unordered_map<int, DirectoryEntry>::iterator it;
        std::cout << "Directory Table ->" << std::endl;
        for (it = directoryTable.begin(); it != directoryTable.end(); it++){
            std::cout << it->first << " - " << it->second.start_index << " : " << it->second.file_size << " | " ;
        }
        std::cout << std::endl << "FAT ->" << std::endl;
        for(int i = 0; i < 50; i++) std::cout << FAT[i] << " ";
        std::cout << std::endl;
    }
    std::cout << "------------------------------------------------------" << std::endl;

    
    std::cout << "-----------------------Extend Test---------------------" << std::endl;
    for(int i = 0; i < 32; i++){
        a->extend(i, 2);
        std::cout << std::endl;
        std::unordered_map<int, DirectoryEntry>::iterator it;
        for (it = directoryTableContigious.begin(); it != directoryTableContigious.end(); it++){
            std::cout << it->first << " - " << it->second.start_index << " : " << it->second.file_size << " | " ;
        }
        std::cout << std::endl;
    }
    
    std::cout << "------------------------------------------------------" << std::endl;
    
    std::cout << "-----------------------Extend Test 2---------------------" << std::endl;
   
    a->extend(0, 32);
    std::cout << std::endl;
    std::unordered_map<int, DirectoryEntry>::iterator it;
    for (it = directoryTableContigious.begin(); it != directoryTableContigious.end(); it++){
        std::cout << it->first << " - " << it->second.start_index << " : " << it->second.file_size << " | " ;
    }
    std::cout << std::endl;
    
    
    std::cout << "------------------------------------------------------" << std::endl;
    
    
    std::cout << "-----------------------Defrag Test---------------------" << std::endl;
    std::cout << "Before: ";
    for(int i = 0; i < 80; i++){
        std::cout << directoryContent[i] << " ";
    }
    a->defragmentation();
    std::cout << std::endl << "After: ";
    for(int i = 0; i < 80; i++){
        std::cout << directoryContent[i] << " ";
    }
    std::cout << std::endl;
    for (it = directoryTableContigious.begin(); it != directoryTableContigious.end(); it++){
        std::cout << it->first << " - " << it->second.start_index << " : " << it->second.file_size << " | " ;
    }
    std::cout << std::endl;
    
    std::cout << "------------------------------------------------------" << std::endl;

    */
    
    return 0;
}
