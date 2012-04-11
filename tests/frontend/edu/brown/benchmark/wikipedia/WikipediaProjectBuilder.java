/*******************************************************************************
 * oltpbenchmark.com
 *  
 *  Project Info:  http://oltpbenchmark.com
 *  Project Members:  	Carlo Curino <carlo.curino@gmail.com>
 * 				Evan Jones <ej@evanjones.ca>
 * 				DIFALLAH Djellel Eddine <djelleleddine.difallah@unifr.ch>
 * 				Andy Pavlo <pavlo@cs.brown.edu>
 * 				CUDRE-MAUROUX Philippe <philippe.cudre-mauroux@unifr.ch>  
 *  				Yang Zhang <yaaang@gmail.com> 
 * 
 *  This library is free software; you can redistribute it and/or modify it under the terms
 *  of the GNU General Public License as published by the Free Software Foundation;
 *  either version 3.0 of the License, or (at your option) any later version.
 * 
 *  This library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See the GNU Lesser General Public License for more details.
 ******************************************************************************/
package edu.brown.benchmark.wikipedia;

import edu.brown.benchmark.AbstractProjectBuilder;
import edu.brown.benchmark.BenchmarkComponent;
import edu.brown.benchmark.wikipedia.procedures.AddWatchList;
import edu.brown.benchmark.wikipedia.procedures.GetPageAnonymous;
import edu.brown.benchmark.wikipedia.procedures.GetPageAuthenticated;
import edu.brown.benchmark.wikipedia.procedures.RemoveWatchList;
import edu.brown.benchmark.wikipedia.procedures.UpdatePage;

public class WikipediaProjectBuilder extends AbstractProjectBuilder {
    
    // REQUIRED: Retrieved via reflection by BenchmarkController
    public static final Class<? extends BenchmarkComponent> m_clientClass = WikipediaClient.class;
 
    // REQUIRED: Retrieved via reflection by BenchmarkController
    public static final Class<? extends BenchmarkComponent> m_loaderClass = WikipediaLoader.class;
 
    public static final Class<?> PROCEDURES[] = new Class<?>[] {
        AddWatchList.class,
        GetPageAnonymous.class,
        GetPageAuthenticated.class,
        RemoveWatchList.class,
        UpdatePage.class,
    };
    
    /**
     * FIXME how the schemas are partitioned...
     */
    public static final String PARTITIONING[][] = new String[][] {
        { WikipediaConstants.TABLENAME_LOGGING, "log_id" },
        { WikipediaConstants.TABLENAME_PAGE, "page_id" },
        { WikipediaConstants.TABLENAME_PAGE_RESTRICTIONS, "pr_page" },
        { WikipediaConstants.TABLENAME_RECENTCHANGES, "rc_id" },
        { WikipediaConstants.TABLENAME_REVISION, "rev_page" },
        { WikipediaConstants.TABLENAME_TEXT, "old_page" },
        { WikipediaConstants.TABLENAME_WATCHLIST, "wl_user" },
        { WikipediaConstants.TABLENAME_USER, "user_id" }
    };
 
    public WikipediaProjectBuilder() {
        super("wikipedia", WikipediaProjectBuilder.class, PROCEDURES, PARTITIONING);
  
    }
    
    public void addDefaultProcedures() {
        addProcedures(PROCEDURES);
    }
    
    public void addDefaultSchema() {
        addSchema(this.getDDLURL(true));
    }

}
