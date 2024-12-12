# Ring Database
## Overview
This repository contains the implementation of a distributed database system with a ring topology with added shortcuts (chords). 
The project, developed in C, focuses designing and implementing systems programming concepts through a dynamic and efficient distributed architecture. 
The nodes of the ring manage unique objects, communicate over TCP/UDP, and support efficient key lookups and dynamic changes.  

## Features
The system implements a Ring Database, allowing efficient management of objects and nodes through the following functionalities:  
- Dynamic Node Management: Nodes can join or leave the ring, redistributing objects appropriately to maintain consistency.  
- key Lookups: Supports optimized key searches using optional shortcuts (chords).  
- Distributed Architecture: Nodes communicate using TCP and UDP protocols.
- Position Discovery: New nodes can join without knowing their position, dynamically determining their predecessor in the ring.  

## Features and Commands  
The system includes an intuitive command-line interface for managing nodes and objects.  
- **Ring Creation**:  
  - `new`: Initializes a new ring with a single node.  
- **Node Entry and Exit**:  
  - `pentry`: Adds a node to the ring with a known predecessor.  
  - `bentry`: Adds a node without prior knowledge of its predecessor.  
  - `leave`: Removes a node from the ring, transferring its objects to the predecessor.  
- **Key Search and Lookup**:  
  - `find <key>`: Locates the node responsible for the specified key.  
- **Chord Management**:  
  - `chord`: Creates a shortcut to another node for faster traversal.  
  - `echord`: Removes an existing chord.  
- **Node State Display**:  
  - `show`: Displays the current node's details, including its predecessor, successor, and chords.  
- **Application Termination**:  
  - `exit`: Gracefully closes the node application.

## Technical Details  
- **Communication Protocols**:  
  - **TCP**: Used for reliable communication between nodes and their immediate neighbors (successor and predecessor).  
  - **UDP**: Used for lightweight communication, including chord and position discovery protocols.  
- **Message Types**:  
  - Key search (`FND`, `RSP`), node entry/exit (`PRED`, `SELF`), and discovery (`EFND`, `EPRED`).
  
## How It Works  
The system operates by organizing nodes and objects in a circular structure:  
1. **Node Responsibilities**: Each node is responsible for objects with keys between its own key and that of its successor.  
2. **Joining the Ring**: Nodes can join dynamically, updating their predecessor and successor, and redistributing objects to maintain balance.  
3. **Leaving the Ring**: Nodes exiting the system transfer their objects to their predecessor, ensuring no data loss.  
4. **Efficient Lookups**: Through the use of chords, nodes can reduce the traversal distance during key searches.  
