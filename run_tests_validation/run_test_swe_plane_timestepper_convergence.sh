#! /bin/bash


echo "***********************************************"
echo "Running convergence tests"
echo "***********************************************"

# set close affinity of threads
export OMP_PROC_BIND=close


BASEDIR=`pwd`

cd "$BASEDIR/run_test_swe_plane_timestepper_convergence"
./compile.sh || exit 1



./cleanup.sh

# encoding: group | tsm | order1 | order2 | rexi_direct_solution

#if true; then
if false; then
	# 4th order accurate scheme
#	./jobs_create.py ln4 ln_erk 4 4 0
	./jobs_create.py ln4 ln_etdrk 4 4 1
else
	# 1st order linear
	./jobs_create.py l1 l_irk 1 0 0
	./jobs_create.py l1 l_erk 1 0 0

	# 2nd order linear
	./jobs_create.py l2 l_cn 2 0 0
	./jobs_create.py l2 l_erk 2 0 0

	# 1st order nonlinear
	./jobs_create.py ln1 ln_erk 1 1 0
	./jobs_create.py ln1 ln_etdrk 1 1 1
	./jobs_create.py ln1 l_erk_n_erk 1 1 0
	./jobs_create.py ln1 l_irk_n_erk 1 1 0
	./jobs_create.py ln1 l_rexi_n_erk 1 1 1

	# 2nd order nonlinear
	./jobs_create.py ln2 ln_erk 2 2 0
	./jobs_create.py ln2 ln_etdrk 2 2 1
	./jobs_create.py ln2 l_cn_n_erk 2 2 0
	./jobs_create.py ln2 l_erk_n_erk 2 2 0
	./jobs_create.py ln2 l_rexi_n_erk 2 2 1

	# 2nd order nonlinear, SL-REXI related
	./jobs_create.py ln2space ln_erk 2 2 0
	./jobs_create.py ln2space l_cn_na_sl_nd_settls 2 2 0

	# 4th order accurate scheme
	./jobs_create.py ln4 ln_erk 4 4 0
	./jobs_create.py ln4 ln_etdrk 4 4 1
fi



./jobs_run.sh || exit 1



echo "***********************************************"
echo " POSTPROCESSING "
echo "***********************************************"
./postprocessing.py || exit 1


./cleanup.sh

echo "***********************************************"
echo "***************** FIN *************************"
echo "***********************************************"

