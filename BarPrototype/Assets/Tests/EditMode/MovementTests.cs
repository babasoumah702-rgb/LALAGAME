using NUnit.Framework;
using UnityEngine;

namespace BarPrototype.Tests
{
    public sealed class MovementTests
    {
        private static readonly Quaternion View = Quaternion.Euler(35,45,0);
        [Test] public void ZeroInputDoesNotMove() => Assert.That(PlayerMotor.ScreenDirection(Vector2.zero,View),Is.EqualTo(Vector3.zero));
        [Test] public void DiagonalInputIsNotFaster()
        {
            var straight=PlayerMotor.ScreenDirection(Vector2.up,View).magnitude;
            var diagonal=PlayerMotor.ScreenDirection(Vector2.one,View).magnitude;
            Assert.That(diagonal,Is.EqualTo(straight).Within(.0001f));
        }
        [Test] public void MovementStaysOnGroundPlane()
        {
            foreach(var input in new[]{Vector2.up,Vector2.down,Vector2.left,Vector2.right,Vector2.one})
                Assert.That(PlayerMotor.ScreenDirection(input,View).y,Is.Zero);
        }
        [Test] public void UpMovesTowardsTopOfScreen()
        {
            var viewDirection=Quaternion.Inverse(View)*PlayerMotor.ScreenDirection(Vector2.up,View);
            Assert.That(viewDirection.y,Is.GreaterThan(0));Assert.That(viewDirection.x,Is.EqualTo(0).Within(.0001f));
        }
        [Test] public void RightMovesTowardsRightOfScreen()
        {
            var viewDirection=Quaternion.Inverse(View)*PlayerMotor.ScreenDirection(Vector2.right,View);
            Assert.That(viewDirection.x,Is.GreaterThan(.99f));
        }
    }
}
