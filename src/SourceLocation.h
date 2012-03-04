//
//  Header.h
//  LightBASIC
//
//  Created by Albert Varaksin on 25/02/2012.
//  Copyright (c) 2012 LightBASIC development team. All rights reserved.
//

#pragma once
namespace lbc {
    
    // The source
    class Source;
    
    /**
     * Represent a source location
     */
    struct SourceLocation
    {
        /// create Source Reference
        SourceLocation(unsigned int line, unsigned short column, unsigned short length)
        : line(line), column(column), length(length)
        {}
        
        // copy contructor
        SourceLocation(const SourceLocation & loc) = default;
        
        // get thhe line
        inline unsigned int getLine() const { return line; }
        
        // get the column
        inline unsigned short getColumn() const { return column; }
        
        // get length
        inline unsigned short getLength() const { return length; }
        
    private:
        unsigned int        line;
        unsigned short        column;
        unsigned short        length;
    };

    
}