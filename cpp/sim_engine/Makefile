#***********************************************
#This file is part of the NRTB project (https://github.com/fpstovall/nrtb_alpha).
#
#    NRTB is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    NRTB is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with NRTB.  If not, see <http://www.gnu.org/licenses/>.
#
#***********************************************

lib: ./bin/nrtb_simulation
    
./bin/nrtb_simulation: ../common/nrtb_common.a
	@echo "============= building simulation engine ==============="
	@mkdir -p ./obj ./lib ./include ./bin
	@ar -r ./lib/sim_engine.a
	@make action=lib doit
	@echo "============= simulation engine complete ==============="

clean:
	@echo "============= cleaning simulation engine ==============="
	@make action=clean doit
	@rm -rf ./obj ./lib ./include ./bin
	@echo "========== simulation engine cleanup complete ==========="

doit:
	cd messages; make ${action}
	cd main; make ${action}
	cd base_object; make ${action}
	cd common_log; make ${action}
	cd sim_core; make ${action}
	cd gravity_effector; make ${action}
	cd hover_effector; make ${action}

../common/nrtb_common.a:
	@cd ../common; make 

